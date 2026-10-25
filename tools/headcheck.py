"""Find playable frames whose head floats off the body (Ryan, 2026-10-25: Krillin's head popped off in his dash).

Reads a BUILT ROM (the relocated sprite index through coop.sym) and, for every sprite id given, every group, facing
and frame, splits the drawn pixels into 4-connected components; a frame is flagged when its second-largest component
is a sizeable blob (>= MIN_PART px) sitting ABOVE the largest one with a gap of empty rows between them: a pasted head
that does not touch its shoulders. Effects (sparks, motion lines) are small and are ignored.

usage: python -I tools/headcheck.py BUILT.gba coop.sym ID[,ID...] [--sheet OUT.png]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sprites as S  # noqa: E402

MIN_PART = 18     # px: a head (Krillin's is ~60), not a spark


def components(px, w, h):
    seen = [[False] * w for _ in range(h)]
    out = []
    for y in range(h):
        for x in range(w):
            if px[y][x] and not seen[y][x]:
                stack = [(x, y)]
                seen[y][x] = True
                pts = []
                while stack:
                    cx, cy = stack.pop()
                    pts.append((cx, cy))
                    for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                        if 0 <= nx < w and 0 <= ny < h and px[ny][nx] and not seen[ny][nx]:
                            seen[ny][nx] = True
                            stack.append((nx, ny))
                out.append(pts)
    out.sort(key=len, reverse=True)
    return out


def floating(px, w, h):
    comps = components(px, w, h)
    if len(comps) < 2 or len(comps[1]) < MIN_PART:
        return None
    body, part = comps[0], comps[1]
    part_bottom = max(y for _, y in part)
    body_top = min(y for _, y in body)
    if part_bottom < body_top:                     # entirely above the body, with empty rows between
        return body_top - part_bottom
    return None


def main():
    rom = open(sys.argv[1], "rb").read()
    sym = {}
    for line in open(sys.argv[2]):
        p = line.split()
        if len(p) == 3:
            sym[p[2]] = int(p[0], 16)
    idx = sym["sprite_index"]
    ids = [int(v, 0) for v in sys.argv[3].split(",")]
    sp = S.Sprites(rom)
    flagged = []
    for sid in ids:
        rec = int.from_bytes(rom[idx - S.ROM_BASE + 4 * sid: idx - S.ROM_BASE + 4 * sid + 4], "little")
        if not sp.is_ptr(rec):
            print(f"{sid}: no record")
            continue
        groups = sp.groups(rec)
        n = 0
        for gi, g in enumerate(groups):
            if not g:
                continue
            for d, arr in enumerate(g):
                if not arr:
                    continue
                for fi in range(16):
                    try:
                        fp = sp.frame_pixels(arr, fi)
                    except Exception:  # noqa: BLE001
                        break
                    if not fp:
                        break
                    w, h, px = fp
                    gap = floating(px, w, h)
                    if gap:
                        flagged.append((sid, gi, d, fi, gap))
                        n += 1
                    if fi > 0 and not sp.valid_descriptor(arr, fi + 1):
                        break
        print(f"{sid}: {n} floating frames")
    for f in flagged:
        print("  sprite %d group %d facing %d frame %d: gap %d rows" % f)


if __name__ == "__main__":
    main()
