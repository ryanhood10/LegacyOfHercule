"""A lying-down frame made from an upright one (docs/sprites.md "Lying poses"), for bodies whose set has no lying
pose (Frieza, General Tao, Cell Jr., Cooler's stock death frame drawn feet-first, the Buu's Fury imports without a
lie-down animation). The upright frame (a hit-stun or recoil pose, its hit sparks dropped) is turned a quarter turn
so the body lies with the head away from the way it faced, on its back or face down (whichever has more of the body in
its lower half: a tail or a cape curls up, not under), trimmed, and set one row below the ground line the upright body stood on,
centred where it stood.

    lie(px, xo, yo, face_left=True) -> (xo, yo, w, h, px)    # px as displayed (no h-flip), w x h a GBA OBJ size

`px` is the frame as displayed ([h][w] palette indices), `xo, yo` its offset from the entity's position. A body
facing left falls back to the right (rotated clockwise: head to the right, front up); facing right, the mirror.
"""

OBJ = [(8, 8), (16, 8), (8, 16), (16, 16), (32, 8), (8, 32), (32, 16), (16, 32), (32, 32), (64, 32), (32, 64),
       (64, 64)]


def obj_size(w, h):
    for ow, oh in sorted(OBJ, key=lambda s: (s[0] * s[1], s)):
        if ow >= w and oh >= h:
            return ow, oh
    return None


def rotate_cw(px):
    h, w = len(px), len(px[0])
    return [[px[h - 1 - x][y] for x in range(h)] for y in range(w)]


def body_only(px):
    """The frame without its effects: 8-connected pieces smaller than a fifth of the largest (hit sparks, shock
    lines) are dropped; a hand or a foot drawn apart from the body is bigger than that."""
    h, w = len(px), len(px[0])
    seen, comps = set(), []
    for y in range(h):
        for x in range(w):
            if px[y][x] and (y, x) not in seen:
                comp, stack = [], [(y, x)]
                seen.add((y, x))
                while stack:
                    cy, cx = stack.pop()
                    comp.append((cy, cx))
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            q = (cy + dy, cx + dx)
                            if 0 <= q[0] < h and 0 <= q[1] < w and px[q[0]][q[1]] and q not in seen:
                                seen.add(q)
                                stack.append(q)
                comps.append(comp)
    if not comps:
        return px
    big = max(len(c) for c in comps)
    out = [[0] * w for _ in range(h)]
    for c in comps:
        if len(c) * 5 >= big:
            for y, x in c:
                out[y][x] = px[y][x]
    return out


def lie(px, xo, yo, face_left=True, max_bytes=None):
    px = body_only(px)
    pts = [(y, x) for y, row in enumerate(px) for x, v in enumerate(row) if v]
    if not pts:
        return xo, yo, len(px[0]), len(px), px
    ground = yo + max(y for y, _ in pts)                      # the feet's row, in entity coordinates
    cx = xo + (min(x for _, x in pts) + max(x for _, x in pts) + 1) / 2
    src = px if face_left else [row[::-1] for row in px]
    r = rotate_cw(src)
    # on the back or face down: whichever puts the body's mass nearer the ground (Cooler's tail, which hangs behind
    # him upright, curled under his back and held him up off the ground; face down it curls up in the air)
    flip = r[::-1]

    def footing(q):          # the drawn pixels in the lower half of the body's rows (a tail curled under: few)
        rows = [y for y, row in enumerate(q) if any(row)]
        mid = (min(rows) + max(rows)) / 2
        return sum(1 for y in rows if y > mid for v in q[y] if v)

    if footing(flip) > footing(r) * 1.25:
        r = flip
    rp = [(y, x) for y, row in enumerate(r) for x, v in enumerate(row) if v]
    y0, y1 = min(y for y, _ in rp), max(y for y, _ in rp)
    x0, x1 = min(x for _, x in rp), max(x for _, x in rp)
    body = [row[x0:x1 + 1] for row in r[y0:y1 + 1]]
    bh, bw = len(body), len(body[0])
    if bw > 64:                                               # a giant longer than an OBJ: keep the middle
        cut = (bw - 64) // 2
        body = [row[cut:cut + 64] for row in body]
        bw = 64
    ow, oh = obj_size(bw, bh)
    if max_bytes and ow * oh > max_bytes:
        raise ValueError(f"lying frame {ow}x{oh} is larger than the record's {max_bytes} bytes")
    out = [[0] * ow for _ in range(oh)]
    left = (ow - bw) // 2
    for y in range(bh):
        for x in range(bw):
            out[oh - bh + y][left + x] = body[y][x]
    if not face_left:
        out = [row[::-1] for row in out]
    nxo = int(round(cx - ow / 2))
    nyo = ground - oh + 1 + 1                                 # one row lower: a body flat on the ground, not on its feet
    return nxo, nyo, ow, oh, out


OBJ_ATTR = {(8, 8): (0, 0), (16, 16): (0, 1), (32, 32): (0, 2), (64, 64): (0, 3), (16, 8): (1, 0), (32, 8): (1, 1),
            (32, 16): (1, 2), (64, 32): (1, 3), (8, 16): (2, 0), (8, 32): (2, 1), (16, 32): (2, 2), (32, 64): (2, 3)}


def attr_for(attr, w, h):
    """A descriptor's attr word (attr0 | attr1 << 16) for a w x h frame drawn as stored (no h-flip)."""
    shape, size = OBJ_ATTR[(w, h)]
    return (attr & 0x0FFF3FFF) | (shape << 14) | (size << 30)          # keeps 8bpp (bit 13); no flip
