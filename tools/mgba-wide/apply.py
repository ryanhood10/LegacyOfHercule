"""Apply the wide-view patch to an mGBA 0.10 source tree (idempotent; every anchor must match).

usage: apply.py <mgba source dir>
Adds: a runtime frame size (up to 720 x 480) read from an EWRAM descriptor the ROM writes,
backgrounds drawn from the game's ROM map chunks (software-wide.c), extended OBJ coordinates
(attr1 bits 9/10 for non-affine sprites), an SDL presenter that scales the current frame size to the
window, and MGBA_SCRIPT=<file.lua> loading in the SDL frontend.
"""
import os
import shutil
import sys

root = sys.argv[1]
here = os.path.dirname(os.path.abspath(__file__))


def edit(rel, pairs, must=True):
    path = os.path.join(root, rel)
    s = open(path, encoding="utf-8").read()
    for old, new in pairs:
        if new in s and old not in s:
            continue  # already applied
        if old not in s:
            if must:
                raise SystemExit(f"{rel}: anchor not found:\n{old}")
            continue
        s = s.replace(old, new, 1)
    open(path, "w", encoding="utf-8", newline="\n").write(s)
    print("patched", rel)


# ---- renderer base: give renderers the GBA pointer (ROM, EWRAM, IWRAM access) -----------------
edit("include/mgba/internal/gba/video.h", [
    # window columns must hold the wide width (up to 720), not 8 bits
    ("struct GBAVideoWindowRegion {\n\tuint8_t end;\n\tuint8_t start;\n};", "struct GBAVideoWindowRegion {\n\tuint16_t end;\n\tuint16_t start;\n};"),
    ("struct GBAVideoRenderer {\n", "struct GBA;\nstruct GBAVideoRenderer {\n"),
    ("\tunion GBAOAM* oam;\n", "\tunion GBAOAM* oam;\n\tstruct GBA* gba;   /* wide view: ROM/RAM access */\n"),
])
edit("src/gba/video.c", [
    ("\trenderer->oam = &video->oam;\n", "\trenderer->oam = &video->oam;\n\trenderer->gba = video->p;\n"),
])

# ---- software renderer header ----------------------------------------------------------------
edit("include/mgba/internal/gba/renderers/video-software.h", [
    ("#define MAX_WINDOW 5\n", """#define MAX_WINDOW 5

/* wide view (Legacy of Goku II co-op fork) */
#define WIDE_MAX_W 720
#define WIDE_MAX_H 480
#define WIDE_CHUNK_CACHE 64
#define WIDE_DESC_OFFSET 0x3FEC0        /* EWRAM 0x0203FEC0: u32 magic, u16 w, u16 h */
#define WIDE_MAGIC 0x45444957           /* "WIDE" */
struct WideChunk {
	uint32_t addr;
	uint32_t stamp;
	uint16_t map[1024];
};
extern int mWideFrameW;
extern int mWideFrameH;
"""),
    ("\tuint32_t row[GBA_VIDEO_HORIZONTAL_PIXELS];\n\tuint32_t spriteLayer[GBA_VIDEO_HORIZONTAL_PIXELS];\n",
     "\tuint32_t row[WIDE_MAX_W];\n\tuint32_t spriteLayer[WIDE_MAX_W];\n"),
    ("\tuint32_t scanlineDirty[5];\n", "\tuint32_t scanlineDirty[(WIDE_MAX_H + 31) / 32];\n"),
    ("\tuint8_t endX;\n\tstruct WindowControl control;\n", "\tuint16_t endX;   /* wide view: up to 720 */\n\tstruct WindowControl control;\n"),
    ("\t} cache[GBA_VIDEO_VERTICAL_PIXELS];\n", "\t} cache[WIDE_MAX_H];\n"),
    ("\tuint8_t lastHighlightAmount;\n};\n", """\tuint8_t lastHighlightAmount;

	int wideW;
	int wideH;
	int curSpriteX;
	int curSpriteY;
	struct WideChunk chunks[WIDE_CHUNK_CACHE];
	uint32_t chunkStamp;
};

void GBAVideoSoftwareRendererWideBeginFrame(struct GBAVideoSoftwareRenderer* renderer);
uint32_t GBAVideoSoftwareRendererWideLayer(struct GBAVideoSoftwareRenderer* renderer, int bg);
void GBAVideoSoftwareRendererDrawBackgroundWide(struct GBAVideoSoftwareRenderer* renderer, struct GBAVideoSoftwareBackground* background, int y, uint32_t layer);
"""),
])

# ---- sprite records carry a decoded x ----------------------------------------------------------
edit("include/mgba/internal/gba/renderers/common.h", [
    ("\tint16_t y;\n\tint16_t endY;\n", "\tint16_t y;\n\tint16_t endY;\n\tint16_t x;\n"),
    ("int GBAVideoRendererCleanOAM(struct GBAObj* oam, struct GBAVideoRendererSprite* sprites, int offsetY);",
     "int GBAVideoRendererCleanOAM(struct GBAObj* oam, struct GBAVideoRendererSprite* sprites, int offsetY, int wideW, int wideH);"),
])
edit("src/gba/renderers/common.c", [
    ("int GBAVideoRendererCleanOAM(struct GBAObj* oam, struct GBAVideoRendererSprite* sprites, int offsetY) {\n\tint i;\n\tint oamMax = 0;\n",
     "int GBAVideoRendererCleanOAM(struct GBAObj* oam, struct GBAVideoRendererSprite* sprites, int offsetY, int wideW, int wideH) {\n\tint i;\n\tint oamMax = 0;\n\tbool wide = wideW > GBA_VIDEO_HORIZONTAL_PIXELS || wideH > GBA_VIDEO_VERTICAL_PIXELS;\n"),
    ("""			if (GBAObjAttributesAGetY(obj.a) >= GBA_VIDEO_VERTICAL_PIXELS && GBAObjAttributesAGetY(obj.a) + height < VIDEO_VERTICAL_TOTAL_PIXELS) {
				continue;
			}
			if (GBAObjAttributesBGetX(obj.b) >= GBA_VIDEO_HORIZONTAL_PIXELS && GBAObjAttributesBGetX(obj.b) + width < 512) {
				continue;
			}
			int y = GBAObjAttributesAGetY(obj.a) + offsetY;
			sprites[oamMax].y = y;
			sprites[oamMax].endY = y + height;
""", """			int x = GBAObjAttributesBGetX(obj.b);
			int y;
			if (wide) {
				y = GBAObjAttributesAGetY(obj.a);
				if (!GBAObjAttributesAIsTransformed(obj.a)) {
					/* attr1 bits 9/10 are free for non-affine sprites: x bit 9, y bit 8 */
					if (obj.b & 0x0200) {
						x += 512;
					}
					if (obj.b & 0x0400) {
						y += 256;
					}
				}
				if (x >= 1024 - 64) {
					x -= 1024;
				}
				if (y >= 512 - 64) {
					y -= 512;
				}
				if (x >= wideW || x + width <= 0 || y >= wideH || y + height <= 0) {
					continue;
				}
				y += offsetY;
			} else {
				if (GBAObjAttributesAGetY(obj.a) >= GBA_VIDEO_VERTICAL_PIXELS && GBAObjAttributesAGetY(obj.a) + height < VIDEO_VERTICAL_TOTAL_PIXELS) {
					continue;
				}
				if (GBAObjAttributesBGetX(obj.b) >= GBA_VIDEO_HORIZONTAL_PIXELS && GBAObjAttributesBGetX(obj.b) + width < 512) {
					continue;
				}
				y = GBAObjAttributesAGetY(obj.a) + offsetY;
				x = ((int32_t) ((uint32_t) x << 23)) >> 23;
			}
			sprites[oamMax].x = x;
			sprites[oamMax].y = y;
			sprites[oamMax].endY = y + height;
"""),
])
edit("src/gba/renderers/gl.c", [
    ("GBAVideoRendererCleanOAM(glRenderer->d.oam->obj, glRenderer->sprites, 0);",
     "GBAVideoRendererCleanOAM(glRenderer->d.oam->obj, glRenderer->sprites, 0, GBA_VIDEO_HORIZONTAL_PIXELS, GBA_VIDEO_VERTICAL_PIXELS);"),
], must=False)

# ---- software-obj.c: use the decoded coordinates in wide mode ----------------------------------
edit("src/gba/renderers/software-obj.c", [
    ("""	int32_t x = (uint32_t) GBAObjAttributesBGetX(sprite->b) << 23;
	x >>= 23;
	x += renderer->objOffsetX;
""", """	bool wide = renderer->wideW > GBA_VIDEO_HORIZONTAL_PIXELS || renderer->wideH > GBA_VIDEO_VERTICAL_PIXELS;
	int32_t x = (uint32_t) GBAObjAttributesBGetX(sprite->b) << 23;
	x >>= 23;
	x += renderer->objOffsetX;
	if (wide) {
		x = renderer->curSpriteX + renderer->objOffsetX;
	}
"""),
    ("	int inY = y - ((int) GBAObjAttributesAGetY(sprite->a) + renderer->objOffsetY);\n",
     "	int inY = y - ((int) GBAObjAttributesAGetY(sprite->a) + renderer->objOffsetY);\n	if (wide) {\n		inY = y - renderer->curSpriteY;\n	}\n"),
    ("		if (inY < 0) {\n			inY += 256;\n		}\n", "		if (inY < 0 && !wide) {\n			inY += 256;\n		}\n"),
    ("		if ((int) GBAObjAttributesAGetY(sprite->a) + height - 256 >= 0) {\n			inY += 256;\n		}\n",
     "		if (!wide && (int) GBAObjAttributesAGetY(sprite->a) + height - 256 >= 0) {\n			inY += 256;\n		}\n"),
])

# ---- video-software.c -------------------------------------------------------------------------
edit("src/gba/renderers/video-software.c", [
    ("#define DIRTY_SCANLINE(R, Y)", "int mWideFrameW = GBA_VIDEO_HORIZONTAL_PIXELS;\nint mWideFrameH = GBA_VIDEO_VERTICAL_PIXELS;\n\n#define DIRTY_SCANLINE(R, Y)"),
    ("\tsoftwareRenderer->objOffsetX = 0;\n\tsoftwareRenderer->objOffsetY = 0;\n",
     "\tsoftwareRenderer->objOffsetX = 0;\n\tsoftwareRenderer->objOffsetY = 0;\n\tsoftwareRenderer->wideW = GBA_VIDEO_HORIZONTAL_PIXELS;\n\tsoftwareRenderer->wideH = GBA_VIDEO_VERTICAL_PIXELS;\n\tmemset(softwareRenderer->chunks, 0, sizeof(softwareRenderer->chunks));\n\tsoftwareRenderer->chunkStamp = 0;\n"),
    # frame start: read the descriptor
    ("""static void GBAVideoSoftwareRendererDrawScanline(struct GBAVideoRenderer* renderer, int y) {
	struct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;

	if (y == GBA_VIDEO_VERTICAL_PIXELS - 1) {""", """static void GBAVideoSoftwareRendererDrawScanline(struct GBAVideoRenderer* renderer, int y) {
	struct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;

	if (y == 0) {
		GBAVideoSoftwareRendererWideBeginFrame(softwareRenderer);
	}
	if (y == GBA_VIDEO_VERTICAL_PIXELS - 1) {"""),
    # forced blank fill
    ("""	if (GBARegisterDISPCNTIsForcedBlank(softwareRenderer->dispcnt)) {
		int x;
		for (x = 0; x < GBA_VIDEO_HORIZONTAL_PIXELS; ++x) {
			row[x] = M_COLOR_WHITE;
		}
		return;
	}
""", """	if (GBARegisterDISPCNTIsForcedBlank(softwareRenderer->dispcnt)) {
		int x;
		for (x = 0; x < softwareRenderer->wideW; ++x) {
			row[x] = M_COLOR_WHITE;
		}
		return;
	}
"""),
    # BG dispatch (mode 0 sites)
    ("""			if (TEST_LAYER_ENABLED(0) && GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt) < 2) {
				GBAVideoSoftwareRendererDrawBackgroundMode0(softwareRenderer, &softwareRenderer->bg[0], y);
			}
			if (TEST_LAYER_ENABLED(1) && GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt) < 2) {
				GBAVideoSoftwareRendererDrawBackgroundMode0(softwareRenderer, &softwareRenderer->bg[1], y);
			}
			if (TEST_LAYER_ENABLED(2)) {
				switch (GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt)) {
				case 0:
					GBAVideoSoftwareRendererDrawBackgroundMode0(softwareRenderer, &softwareRenderer->bg[2], y);
					break;""", """			if (TEST_LAYER_ENABLED(0) && GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt) < 2) {
				_drawMode0(softwareRenderer, 0, y);
			}
			if (TEST_LAYER_ENABLED(1) && GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt) < 2) {
				_drawMode0(softwareRenderer, 1, y);
			}
			if (TEST_LAYER_ENABLED(2)) {
				switch (GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt)) {
				case 0:
					_drawMode0(softwareRenderer, 2, y);
					break;"""),
    ("""			if (TEST_LAYER_ENABLED(3)) {
				switch (GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt)) {
				case 0:
					GBAVideoSoftwareRendererDrawBackgroundMode0(softwareRenderer, &softwareRenderer->bg[3], y);
					break;""", """			if (TEST_LAYER_ENABLED(3)) {
				switch (GBARegisterDISPCNTGetMode(softwareRenderer->dispcnt)) {
				case 0:
					_drawMode0(softwareRenderer, 3, y);
					break;"""),
    # helper definition before drawScanline
    ("static void GBAVideoSoftwareRendererDrawScanline(struct GBAVideoRenderer* renderer, int y) {\n\tstruct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;\n\n\tif (y == 0) {",
     """static inline void _drawMode0(struct GBAVideoSoftwareRenderer* renderer, int bg, int y) {
	if (renderer->wideW > GBA_VIDEO_HORIZONTAL_PIXELS || renderer->wideH > GBA_VIDEO_VERTICAL_PIXELS) {
		uint32_t layer = GBAVideoSoftwareRendererWideLayer(renderer, bg);
		if (layer) {
			GBAVideoSoftwareRendererDrawBackgroundWide(renderer, &renderer->bg[bg], y, layer);
			return;
		}
	}
	GBAVideoSoftwareRendererDrawBackgroundMode0(renderer, &renderer->bg[bg], y);
}

static void GBAVideoSoftwareRendererDrawScanline(struct GBAVideoRenderer* renderer, int y) {
	struct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;

	if (y == 0) {"""),
    # row copy loops
    ("		for (x = 0; x < GBA_VIDEO_HORIZONTAL_PIXELS; x += 4) {\n			row[x] = softwareRenderer->row[x] & (M_COLOR_RED | M_COLOR_BLUE);",
     "		for (x = 0; x < softwareRenderer->wideW; x += 4) {\n			row[x] = softwareRenderer->row[x] & (M_COLOR_RED | M_COLOR_BLUE);"),
    ("		for (x = 0; x < GBA_VIDEO_HORIZONTAL_PIXELS; x += 4) {\n			row[x] = softwareRenderer->row[x];",
     "		for (x = 0; x < softwareRenderer->wideW; x += 4) {\n			row[x] = softwareRenderer->row[x];"),
    ("		memcpy(row, softwareRenderer->row, GBA_VIDEO_HORIZONTAL_PIXELS * sizeof(*row));",
     "		memcpy(row, softwareRenderer->row, softwareRenderer->wideW * sizeof(*row));"),
    # finishFrame: extra rows
    ("""static void GBAVideoSoftwareRendererFinishFrame(struct GBAVideoRenderer* renderer) {
	struct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;

	softwareRenderer->nextY = 0;""", """static void GBAVideoSoftwareRendererFinishFrame(struct GBAVideoRenderer* renderer) {
	struct GBAVideoSoftwareRenderer* softwareRenderer = (struct GBAVideoSoftwareRenderer*) renderer;

	if (softwareRenderer->wideH > GBA_VIDEO_VERTICAL_PIXELS) {
		int y;
		for (y = GBA_VIDEO_VERTICAL_PIXELS; y < softwareRenderer->wideH; ++y) {
			GBAVideoSoftwareRendererDrawScanline(renderer, y);
		}
	}
	mWideFrameW = softwareRenderer->wideW;
	mWideFrameH = softwareRenderer->wideH;

	softwareRenderer->nextY = 0;"""),
    # window split end
    ("		splits[1].h.end = GBA_VIDEO_HORIZONTAL_PIXELS;", "		splits[1].h.end = softwareRenderer->wideW;"),
    # preprocess buffer
    ("""	for (x = 0; x < GBA_VIDEO_HORIZONTAL_PIXELS; x += 4) {
		softwareRenderer->spriteLayer[x] = FLAG_UNWRITTEN;""", """	for (x = 0; x < softwareRenderer->wideW; x += 4) {
		softwareRenderer->spriteLayer[x] = FLAG_UNWRITTEN;"""),
    ("	softwareRenderer->windows[0].endX = GBA_VIDEO_HORIZONTAL_PIXELS;\n	softwareRenderer->nWindows = 1;",
     "	softwareRenderer->windows[0].endX = softwareRenderer->wideW;\n	softwareRenderer->nWindows = 1;"),
    # sprite layer: CleanOAM args, wide y test, current sprite coords
    ("			renderer->oamMax = GBAVideoRendererCleanOAM(renderer->d.oam->obj, renderer->sprites, renderer->objOffsetY);",
     "			renderer->oamMax = GBAVideoRendererCleanOAM(renderer->d.oam->obj, renderer->sprites, renderer->objOffsetY, renderer->wideW, renderer->wideH);"),
    ("""			if ((y < sprite->y && (sprite->endY - 256 < 0 || y >= sprite->endY - 256)) || y >= sprite->endY) {
				continue;
			}
""", """			if (renderer->wideW > GBA_VIDEO_HORIZONTAL_PIXELS || renderer->wideH > GBA_VIDEO_VERTICAL_PIXELS) {
				if (y < sprite->y || y >= sprite->endY) {
					continue;
				}
			} else if ((y < sprite->y && (sprite->endY - 256 < 0 || y >= sprite->endY - 256)) || y >= sprite->endY) {
				continue;
			}
			renderer->curSpriteX = sprite->x;
			renderer->curSpriteY = sprite->y;
"""),
])
# the OAM dirty flag must also be re-evaluated when the frame size changes; simplest: always clean OAM when wide
edit("src/gba/renderers/video-software.c", [
    ("		if (renderer->oamDirty) {\n			renderer->oamMax = GBAVideoRendererCleanOAM(",
     "		if (renderer->oamDirty || renderer->wideW > GBA_VIDEO_HORIZONTAL_PIXELS || renderer->wideH > GBA_VIDEO_VERTICAL_PIXELS) {\n			renderer->oamMax = GBAVideoRendererCleanOAM("),
])

# ---- new source file --------------------------------------------------------------------------
shutil.copyfile(os.path.join(here, "software-wide.c"), os.path.join(root, "src/gba/renderers/software-wide.c"))
print("copied software-wide.c")
edit("src/gba/renderers/software-private.h", [
    ("#include <mgba/internal/gba/renderers/video-software.h>\n",
     "#include <mgba/internal/gba/renderers/video-software.h>\n#include <limits.h>\n#include <string.h>\n"),
], must=False)

# ---- library source list + SDL frontend always uses the software presenter ---------------------
edit("src/gba/CMakeLists.txt", [
    ("\trenderers/software-bg.c\n", "\trenderers/software-bg.c\n\trenderers/software-wide.c\n"),
])
edit("src/platform/sdl/main.c", [
    ("""#ifdef BUILD_GL
	if (mSDLGLCommonInit(&renderer)) {
		mSDLGLCreate(&renderer);
	} else
#elif defined(BUILD_GLES2) || defined(USE_EPOXY)
#ifdef BUILD_RASPI
	mRPIGLCommonInit(&renderer);
#else
	if (mSDLGLCommonInit(&renderer))
#endif
	{
		mSDLGLES2Create(&renderer);
	} else
#endif
	{
		mSDLSWCreate(&renderer);
	}
""", """	mSDLSWCreate(&renderer);   /* wide view: the software presenter scales the current frame size */
"""),
])

# ---- SDL presenter: texture at the maximum size, present the current frame size -----------------
edit("src/platform/sdl/sw-sdl2.c", [
    ("static bool mSDLSWInit(struct mSDLRenderer* renderer);",
     "#include <mgba/internal/gba/renderers/video-software.h>\n\nstatic bool mSDLSWInit(struct mSDLRenderer* renderer);"),
    ("\tunsigned width, height;\n\trenderer->core->desiredVideoDimensions(renderer->core, &width, &height);\n\trenderer->window = SDL_CreateWindow(",
     "\tunsigned width, height;\n\trenderer->core->desiredVideoDimensions(renderer->core, &width, &height);\n\twidth = WIDE_MAX_W;\n\theight = WIDE_MAX_H;\n\trenderer->window = SDL_CreateWindow("),
    ("\t\t\tSDL_RenderCopy(renderer->sdlRenderer, renderer->sdlTex, 0, 0);\n",
     "\t\t\tSDL_Rect src = { 0, 0, mWideFrameW, mWideFrameH };\n\t\t\tmP2PadPoll(renderer->core);\n\t\t\tSDL_RenderCopy(renderer->sdlRenderer, renderer->sdlTex, &src, 0);\n"),
    ("static bool mSDLSWInit(struct mSDLRenderer* renderer);",
     "void mP2PadPoll(struct mCore* core);\n\nstatic bool mSDLSWInit(struct mSDLRenderer* renderer);"),
])

# ---- SDL frontend: MGBA_SCRIPT=<lua file> ------------------------------------------------------
edit("src/platform/sdl/main.c", [
    ("#ifdef ENABLE_SCRIPTING\n#include <mgba/core/scripting.h>\n",
     "#ifdef ENABLE_SCRIPTING\n#include <mgba/core/scripting.h>\n#include <mgba/script/context.h>\n#include <mgba/core/log.h>\n#include <stdlib.h>\n"),
    ("""#ifdef ENABLE_SCRIPTING
	struct mScriptBridge* bridge = mScriptBridgeCreate();
""", """#ifdef ENABLE_SCRIPTING
	struct mScriptBridge* bridge = mScriptBridgeCreate();
	struct mScriptContext scriptContext;
	const char* scriptPath = getenv("MGBA_SCRIPT");
	if (scriptPath) {
		mScriptContextInit(&scriptContext);
		mScriptContextAttachStdlib(&scriptContext);
		mScriptContextAttachSocket(&scriptContext);
		mScriptContextRegisterEngines(&scriptContext);
		mScriptContextAttachLogger(&scriptContext, &_logger.d);
	}
"""),
    # load the script once the core thread runs and has attached the context (as the Qt frontend does)
    ("""	bool didFail = !mCoreThreadStart(&thread);

	if (!didFail) {
""", """	bool didFail = !mCoreThreadStart(&thread);

	if (!didFail) {
#ifdef ENABLE_SCRIPTING
		if (scriptPath) {
			mCoreThreadInterrupt(&thread);
			if (!mScriptContextLoadFile(&scriptContext, scriptPath)) {
				printf("Could not load script %s\\n", scriptPath);
			} else {
				printf("wide: loaded script %s\\n", scriptPath);
			}
			mCoreThreadContinue(&thread);
		}
#endif
"""),
    ("#ifdef ENABLE_SCRIPTING\n\tmScriptBridgeDestroy(bridge);\n",
     "#ifdef ENABLE_SCRIPTING\n\tif (scriptPath) {\n\t\tmScriptContextDeinit(&scriptContext);\n\t}\n\tmScriptBridgeDestroy(bridge);\n"),
    # the core thread attaches the context itself (and fires start/frame callbacks) when it has one
    ("\tthread.logger.logger = &_logger.d;\n",
     "\tthread.logger.logger = &_logger.d;\n#ifdef ENABLE_SCRIPTING\n\tif (scriptPath) {\n\t\tthread.scriptContext = &scriptContext;\n\t}\n#endif\n"),
])
# ---- player-2 pad over TCP in the SDL frontend (no Lua needed) ----------------------------------
shutil.copyfile(os.path.join(here, "p2pad.c"), os.path.join(root, "src/platform/sdl/p2pad.c"))
print("copied p2pad.c")
edit("src/platform/sdl/CMakeLists.txt", [
    ("set(MAIN_SRC ${CMAKE_SOURCE_DIR}/src/platform/sdl/main.c)",
     "set(MAIN_SRC ${CMAKE_SOURCE_DIR}/src/platform/sdl/main.c ${CMAKE_SOURCE_DIR}/src/platform/sdl/p2pad.c)"),
])
edit("src/platform/sdl/main.c", [
    ("static struct mStandardLogger _logger;\n",
     "static struct mStandardLogger _logger;\nvoid mP2PadInit(void);\nvoid mP2PadDeinit(void);\n"),
    ("\tbool didFail = !mCoreThreadStart(&thread);\n",
     "\tmP2PadInit();\n\tbool didFail = !mCoreThreadStart(&thread);\n"),
    ("#ifdef ENABLE_SCRIPTING\n\tif (scriptPath) {\n\t\tmScriptContextDeinit(&scriptContext);\n\t}\n",
     "\tmP2PadDeinit();\n#ifdef ENABLE_SCRIPTING\n\tif (scriptPath) {\n\t\tmScriptContextDeinit(&scriptContext);\n\t}\n"),
])
print("done")
