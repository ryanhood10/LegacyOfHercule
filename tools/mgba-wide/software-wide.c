/* Wide view for the Legacy of Goku II co-op ROM: draws a text background of any width straight
 * from the game's compressed map chunks in ROM instead of the 256 x 256 tilemap the game streams
 * into VRAM. Part of the mGBA fork patch in tools/mgba-wide (MPL-2.0 like the rest of mGBA). */
#include "gba/renderers/software-private.h"

#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/memory.h>

#define LAYER_TABLE 0x0300107C   /* IWRAM: four BG layer object pointers */
#define WORLD_OBJ   0x03001090   /* IWRAM: world object (+0xC/+0x10 view origin px, +0x24/+0x28 shake) */

static inline uint32_t _rd32(const uint8_t* base, uint32_t off) {
	uint32_t v;
	memcpy(&v, base + off, 4);
	return v;
}

static inline uint16_t _rd16(const uint8_t* base, uint32_t off) {
	uint16_t v;
	memcpy(&v, base + off, 2);
	return v;
}

/* ---- the game's IWRAM decompressor (ROM 0x087FD6BC, entry 0x03000000), ported ------------------ */
struct WideBits {
	const uint8_t* data;
	size_t size;
	size_t pos;
	uint32_t buf;
	int left;
};

static inline void _bitsLoad(struct WideBits* b) {
	if (b->pos + 4 <= b->size) {
		b->buf = _rd32(b->data, b->pos);
	} else {
		b->buf = 0;
	}
	b->pos += 4;
}

static inline int _bit(struct WideBits* b) {
	if (--b->left < 0) {
		_bitsLoad(b);
		b->left = 31;
	}
	int v = b->buf >> 31;
	b->buf <<= 1;
	return v;
}

static inline uint32_t _bits(struct WideBits* b, int n) {
	if (n == 0) {
		return 0;
	}
	if (b->left == 0) {
		_bitsLoad(b);
		b->left = 32;
	}
	int take = n < b->left ? n : b->left;
	uint32_t v = b->buf >> (32 - take);
	b->buf <<= take;
	b->left -= take;
	n -= take;
	if (n) {
		_bitsLoad(b);
		b->left = 32 - n;
		v = (v << n) | (b->buf >> b->left);
		b->buf <<= n;
	}
	return v;
}

static inline uint32_t _gamma(struct WideBits* b) {
	uint32_t v = 1;
	for (;;) {
		v = (v << 1) | _bit(b);
		if (!_bit(b)) {
			return v;
		}
	}
}

/* Returns bytes written (at most outSize). litBits is the chunk header's word 0 (1 or 2). */
static size_t _decompress(const uint8_t* src, size_t srcSize, int litBits, uint8_t* out, size_t outSize) {
	struct WideBits b = { src, srcSize, 0, 0, 0 };
	size_t n = 0;
	int offBits = 8;
	uint32_t lastOff = 1;
	uint32_t base = 0;
	int guard = 0;
#define PUT(v) do { if (n < outSize) { out[n] = (uint8_t) (v); } ++n; } while (0)
#define COPY(dist, len) do { uint32_t _i; for (_i = 0; _i < (len); ++_i) { uint8_t _c = (n >= (dist) && n - (dist) < outSize) ? out[n - (dist)] : 0; PUT(_c); } } while (0)
	while (n < outSize && ++guard < 1000000) {
		if (_bit(&b)) {
			PUT(_bits(&b, litBits) + base);
			continue;
		}
		if (_bit(&b)) {
			uint32_t g = _gamma(&b);
			if (g == 2) {
				uint32_t len = _gamma(&b);
				COPY(lastOff, len);
				continue;
			}
			uint32_t hi = g - 3;
			uint32_t lo = _bits(&b, offBits);
			lastOff = lo + (hi << offBits);
			uint32_t len = _gamma(&b);
			if (lastOff >= 0x10000) {
				len += 3;
			} else if (lastOff >= 0x37FF) {
				len += 2;
			} else if (lastOff >= 0x27F) {
				len += 1;
			} else if (lastOff <= 127) {
				len += 4;
			}
			if (!lastOff) {
				break;
			}
			COPY(lastOff, len);
			continue;
		}
		if (_bit(&b)) {
			int v = (int) _bits(&b, 4) - 1;
			if (v == 0) {
				PUT(0);
			} else if (v > 0) {
				COPY((uint32_t) v, 1);
			} else if (_bit(&b)) {
				do {
					int i;
					for (i = 0; i < 256; ++i) {
						PUT(_bits(&b, 8));
					}
				} while (_bit(&b));
			} else {
				base = 0;
				litBits = 7 + _bit(&b);
				if (litBits != 8) {
					base = _bits(&b, 8);
				}
			}
			continue;
		}
		uint32_t o = _bits(&b, 7);
		if (o) {
			lastOff = o;
			uint32_t len = _bits(&b, 2) + 2;   /* evaluated once: COPY re-evaluates its length */
			COPY(lastOff, len);
			continue;
		}
		uint32_t k = _bits(&b, 2);
		if (k == 0) {
			break;
		}
		offBits = _bits(&b, k + 3);
	}
#undef PUT
#undef COPY
	return n < outSize ? n : outSize;
}

/* ---- chunk cache --------------------------------------------------------------------------- */
static const uint16_t* _chunk(struct GBAVideoSoftwareRenderer* renderer, uint32_t addr) {
	struct GBA* gba = renderer->d.gba;
	if (!gba || !gba->memory.rom || addr < 0x08000000 || addr + 8 > 0x08000000 + gba->memory.romSize) {
		return NULL;
	}
	int i;
	int victim = 0;
	for (i = 0; i < WIDE_CHUNK_CACHE; ++i) {
		struct WideChunk* c = &renderer->chunks[i];
		if (c->addr == addr) {
			c->stamp = ++renderer->chunkStamp;
			return c->map;
		}
		if (c->stamp < renderer->chunks[victim].stamp) {
			victim = i;
		}
	}
	struct WideChunk* c = &renderer->chunks[victim];
	const uint8_t* rom = (const uint8_t*) gba->memory.rom;
	uint32_t off = addr - 0x08000000;
	uint32_t kind = _rd32(rom, off);
	uint32_t size = _rd32(rom, off + 4);
	memset(c->map, 0, sizeof(c->map));
	if (kind == 0) {
		if (size > sizeof(c->map)) {
			size = sizeof(c->map);
		}
		if (off + 8 + size <= gba->memory.romSize) {
			memcpy(c->map, rom + off + 8, size);
		}
	} else if (kind == 1 || kind == 2) {
		_decompress(rom + off + 8, gba->memory.romSize - off - 8, kind, (uint8_t*) c->map, sizeof(c->map));
	}
	c->addr = addr;
	c->stamp = ++renderer->chunkStamp;
	return c->map;
}

/* ---- per-frame descriptor ------------------------------------------------------------------ */
void GBAVideoSoftwareRendererWideBeginFrame(struct GBAVideoSoftwareRenderer* renderer) {
	int oldW = renderer->wideW, oldH = renderer->wideH;
	renderer->wideW = GBA_VIDEO_HORIZONTAL_PIXELS;
	renderer->wideH = GBA_VIDEO_VERTICAL_PIXELS;
	if (oldW != renderer->wideW || oldH != renderer->wideH) {
		memset(renderer->scanlineDirty, 0xFF, sizeof(renderer->scanlineDirty));
	}
	struct GBA* gba = renderer->d.gba;
	if (!gba || !gba->memory.wram) {
		return;
	}
	const uint8_t* wram = (const uint8_t*) gba->memory.wram;
	if (_rd32(wram, WIDE_DESC_OFFSET) != WIDE_MAGIC) {
		return;
	}
	int w = _rd16(wram, WIDE_DESC_OFFSET + 4);
	int h = _rd16(wram, WIDE_DESC_OFFSET + 6);
	if (w < GBA_VIDEO_HORIZONTAL_PIXELS || w > WIDE_MAX_W || h < GBA_VIDEO_VERTICAL_PIXELS || h > WIDE_MAX_H) {
		return;
	}
	if (oldW != (w & ~3) || oldH != h) {
		/* the scanline cache only redraws lines whose registers changed; a new size needs them all */
		memset(renderer->scanlineDirty, 0xFF, sizeof(renderer->scanlineDirty));
	}
	renderer->wideW = w & ~3;
	renderer->wideH = h;
}

/* The layer object for a hardware background, or 0 when it has no chunked map (then the stock
 * VRAM drawer is used, which keeps BG0's dialogue text and the HUD working at native positions). */
uint32_t GBAVideoSoftwareRendererWideLayer(struct GBAVideoSoftwareRenderer* renderer, int bg) {
	struct GBA* gba = renderer->d.gba;
	if (!gba || !gba->memory.iwram || !gba->memory.wram || !gba->memory.rom) {
		return 0;
	}
	const uint8_t* iwram = (const uint8_t*) gba->memory.iwram;
	uint32_t layer = _rd32(iwram, LAYER_TABLE - 0x03000000 + bg * 4);
	if (layer < 0x02000000 || layer + 0x28 >= 0x02040000) {
		return 0;
	}
	const uint8_t* wram = (const uint8_t*) gba->memory.wram;
	if ((_rd32(wram, layer - 0x02000000 + 4) & 0xFF) != (unsigned) bg) {
		return 0;
	}
	uint32_t hdr = _rd32(wram, layer - 0x02000000 + 8);
	if (hdr < 0x08000000 || hdr + 0x20 >= 0x08000000 + gba->memory.romSize) {
		return 0;
	}
	const uint8_t* rom = (const uint8_t*) gba->memory.rom;
	int w = rom[hdr - 0x08000000 + 0x14];
	int h = rom[hdr - 0x08000000 + 0x15];
	if (w * h <= 1 && _rd32(rom, hdr - 0x08000000 + 0x18) == 0) {
		return 0;
	}
	return layer;
}

void GBAVideoSoftwareRendererDrawBackgroundWide(struct GBAVideoSoftwareRenderer* renderer, struct GBAVideoSoftwareBackground* background, int y, uint32_t layer) {
	struct GBA* gba = renderer->d.gba;
	const uint8_t* iwram = (const uint8_t*) gba->memory.iwram;
	const uint8_t* wram = (const uint8_t*) gba->memory.wram;
	const uint8_t* rom = (const uint8_t*) gba->memory.rom;
	uint32_t hdr = _rd32(wram, layer - 0x02000000 + 8) - 0x08000000;
	int32_t scaleX = (int32_t) _rd32(rom, hdr + 4);
	int32_t scaleY = (int32_t) _rd32(rom, hdr + 8);
	int32_t offX = (int32_t) _rd32(rom, hdr + 0xC);
	int32_t offY = (int32_t) _rd32(rom, hdr + 0x10);
	int gridW = rom[hdr + 0x14];
	int gridH = rom[hdr + 0x15];
	uint32_t table = hdr + 0x18;
	int32_t worldX = (int32_t) _rd32(iwram, WORLD_OBJ - 0x03000000 + 0xC);
	int32_t worldY = (int32_t) _rd32(iwram, WORLD_OBJ - 0x03000000 + 0x10);
	int32_t shakeX = (int32_t) _rd32(iwram, WORLD_OBJ - 0x03000000 + 0x24);
	int32_t shakeY = (int32_t) _rd32(iwram, WORLD_OBJ - 0x03000000 + 0x28);
	int32_t scrollX = (worldX * scaleX + offX + shakeX) >> 10;
	int32_t scrollY = (worldY * scaleY + offY + shakeY) >> 10;

	uint32_t flags = background->flags;
	color_t* mainPalette = renderer->normalPalette;
	if (renderer->d.highlightAmount && background->highlight) {
		mainPalette = renderer->highlightPalette;
	}
	if (background->variant) {
		mainPalette = renderer->variantPalette;
		if (renderer->d.highlightAmount && background->highlight) {
			mainPalette = renderer->highlightVariantPalette;
		}
	}
	const uint8_t* vram = (const uint8_t*) renderer->d.vram;
	bool blend = flags & FLAG_TARGET_2;
	int32_t inY = scrollY + y;
	int cy = inY >> 8;
	if (cy < 0 || cy >= gridH) {
		return;
	}
	int tileRow = (inY >> 3) & 31;
	int py = inY & 7;

	int x;
	int32_t wx = scrollX + renderer->start;
	const uint16_t* chunk = NULL;
	int chunkX = INT_MIN;
	for (x = renderer->start; x < renderer->end; ++x, ++wx) {
		int cx = wx >> 8;
		if (cx != chunkX) {
			chunkX = cx;
			chunk = NULL;
			if (cx >= 0 && cx < gridW) {
				uint32_t caddr = _rd32(rom, table + (cy * gridW + cx) * 4);
				if (caddr) {
					chunk = _chunk(renderer, caddr);
				}
			}
		}
		if (!chunk) {
			continue;
		}
		uint32_t current = renderer->row[x];
		if (!IS_WRITABLE(current)) {
			continue;
		}
		uint16_t entry = chunk[tileRow * 32 + ((wx >> 3) & 31)];
		int px = wx & 7;
		if (GBA_TEXT_MAP_HFLIP(entry)) {
			px = 7 - px;
		}
		int ty = GBA_TEXT_MAP_VFLIP(entry) ? 7 - py : py;
		unsigned tile = GBA_TEXT_MAP_TILE(entry);
		int pixelData;
		uint32_t charBase;
		if (!background->multipalette) {
			charBase = background->charBase + tile * 0x20 + ty * 4 + (px >> 1);
			if (charBase >= 0x10000) {
				continue;
			}
			pixelData = vram[charBase];
			pixelData = (px & 1) ? (pixelData >> 4) : (pixelData & 0xF);
			if (!pixelData) {
				continue;
			}
			pixelData += GBA_TEXT_MAP_PALETTE(entry) << 4;
		} else {
			charBase = background->charBase + tile * 0x40 + ty * 8 + px;
			if (charBase >= 0x10000) {
				continue;
			}
			pixelData = vram[charBase];
			if (!pixelData) {
				continue;
			}
		}
		uint32_t color = mainPalette[pixelData] | flags;
		if (blend) {
			_compositeBlendNoObjwin(renderer, &renderer->row[x], color, current);
		} else {
			_compositeNoBlendNoObjwin(renderer, &renderer->row[x], color, current);
		}
	}
}
