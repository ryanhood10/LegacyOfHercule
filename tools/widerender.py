"""Offline reference render of the wide view from a savestate: draws BG1..3 (chunked map layers)
at any size from the ROM chunks, VRAM tile graphics and palette, mirroring tools/mgba-wide/
software-wide.c. Used to validate the fork's renderer logic independently of the emulator.

usage: widerender.py roms/log2.gba build/log2-coop.ss1 out.png [width height]
"""
import struct
import sys
import zlib

sys.path.insert(0, __import__("os").path.dirname(__file__))
import mapchunks as m  # noqa: E402

WORLD = 0x03001090


def png(path, w, h, rgb):
    raw = b"".join(b"\x00" + bytes(rgb[y * w * 3:(y + 1) * w * 3]) for y in range(h))

    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def main():
    rom = open(sys.argv[1], "rb").read()
    st = m.load_state(sys.argv[2])
    out = sys.argv[3]
    W = int(sys.argv[4]) if len(sys.argv) > 4 else 480
    H = int(sys.argv[5]) if len(sys.argv) > 5 else 320
    mem = m.Mem(rom, st)
    vram = st[m.OFF_VRAM:m.OFF_VRAM + 0x18000]
    pram = st[m.OFF_PRAM:m.OFF_PRAM + 0x400]
    pal = [struct.unpack_from("<H", pram, i * 2)[0] for i in range(256)]

    def rgb(c):
        return ((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31)
    io = st[0x1E0:0x1E0 + 0x60]  # not used: BGCNT read from the savestate IO block below
    # IO registers live at state offset 0x1E0? mGBA stores io[] at 0x1E0.. in GBASerializedState? use known values instead
    bgcnt = [0x1C83, 0x1D83, 0x1E83, 0x1F83]
    worldX = struct.unpack("<i", mem.read(WORLD + 0xC, 4))[0]
    worldY = struct.unpack("<i", mem.read(WORLD + 0x10, 4))[0]
    shakeX = struct.unpack("<i", mem.read(WORLD + 0x24, 4))[0]
    shakeY = struct.unpack("<i", mem.read(WORLD + 0x28, 4))[0]
    print(f"world px ({worldX},{worldY}) shake ({shakeX},{shakeY}) view {W}x{H}")
    img = bytearray(W * H * 3)
    bd = rgb(pal[0])
    for i in range(W * H):
        img[i * 3:i * 3 + 3] = bytes(bd)
    layers = m.find_layers(mem)
    chunks = {}
    # draw lowest priority first: BG3, BG2, BG1 (priority bits in BGCNT: all 3,2,1,0?) use index order 3..1
    for bg in (3, 2, 1):
        layer = layers[bg]
        hdr = mem.u32(layer + 8)
        if not (0x08000000 <= hdr < 0x08800000):
            continue
        scaleX = struct.unpack("<i", mem.read(hdr + 4, 4))[0]
        scaleY = struct.unpack("<i", mem.read(hdr + 8, 4))[0]
        offX = struct.unpack("<i", mem.read(hdr + 0xC, 4))[0]
        offY = struct.unpack("<i", mem.read(hdr + 0x10, 4))[0]
        gw, gh = mem.u8(hdr + 0x14), mem.u8(hdr + 0x15)
        scrollX = (worldX * scaleX + offX + shakeX) >> 10
        scrollY = (worldY * scaleY + offY + shakeY) >> 10
        charBase = ((bgcnt[bg] >> 2) & 3) << 14
        print(f"bg{bg}: grid {gw}x{gh} scroll ({scrollX},{scrollY}) charBase {charBase:#x}")
        for y in range(H):
            wy = scrollY + y
            cy = wy >> 8
            if cy < 0 or cy >= gh:
                continue
            trow = (wy >> 3) & 31
            py = wy & 7
            for x in range(W):
                wx = scrollX + x
                cx = wx >> 8
                if cx < 0 or cx >= gw:
                    continue
                caddr = mem.u32(hdr + 0x18 + (cy * gw + cx) * 4)
                if not caddr:
                    continue
                if caddr not in chunks:
                    chunks[caddr] = m.unpack(rom, caddr)
                ch = chunks[caddr]
                e = struct.unpack_from("<H", ch, (trow * 32 + ((wx >> 3) & 31)) * 2)[0]
                px = wx & 7
                if e & 0x400:
                    px = 7 - px
                ty = 7 - py if e & 0x800 else py
                tile = e & 0x3FF
                if bgcnt[bg] & 0x80:   # 256-colour tiles (all four BGs in this game)
                    off = charBase + tile * 64 + ty * 8 + px
                    if off >= 0x10000:
                        continue
                    v = vram[off]
                    if not v:
                        continue
                    c = rgb(pal[v])
                else:
                    off = charBase + tile * 32 + ty * 4 + (px >> 1)
                    if off >= 0x10000:
                        continue
                    v = vram[off]
                    v = (v >> 4) if (px & 1) else (v & 0xF)
                    if not v:
                        continue
                    c = rgb(pal[((e >> 12) << 4) + v])
                i = (y * W + x) * 3
                img[i:i + 3] = bytes(c)
    png(out, W, H, img)
    print("wrote", out)


if __name__ == "__main__":
    main()
