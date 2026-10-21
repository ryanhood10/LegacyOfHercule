"""The Saiyan ship as map art (Ryan, 2026-10-21: "make the ship bigger! double it again! ... more detail ...
give a door to go in the front and actually enter and leave"): Paragus' round ship drawn by code (our own
art, no ROM data), 256 x 160 px, painted into a map layer by tools/mapbuild.py (layer op
{"op": "png", "draw": "shipart", "at": [col, row]}).

A grey metal sphere lit from above and in front (dithered between the BG palette's grey ramp), panel lines
and rivets, a red cockpit dome on top, a crest, a purple ring of lights around its middle (its back arc
showing beside the sphere), swept purple fins, four landing legs, and the open door at the front: a lit
doorway under the ring with a ramp down to the ground. Left-right symmetric (the right half is the left
half mirrored), so the map builder reuses every left tile h-flipped on the right.

usage: shipart.py OUT.png [--x4 OUT_x4.png]     write the art (and a 4x preview)
"""
import math
import sys

W, H = 256, 160
CX = 128                      # mirror axis between pixel 127 and 128

# BG palette colours (exact entries, so the map builder's nearest-colour match keeps them)
GREY = [(0, 0, 0), (41, 41, 41), (65, 65, 65), (90, 90, 90), (123, 123, 123), (156, 156, 156),
        (189, 189, 189), (222, 222, 222), (255, 255, 255)]
PURPLE = [(57, 0, 123), (90, 0, 189), (123, 0, 255), (156, 57, 255), (189, 115, 255), (222, 180, 255),
          (238, 213, 255)]
RED = [(123, 0, 0), (189, 0, 0), (255, 0, 0), (255, 148, 148), (255, 213, 213)]
GLOW = [(123, 57, 0), (205, 98, 0), (238, 115, 0), (255, 172, 90), (255, 205, 148), (255, 238, 213)]
LIGHT = [(255, 255, 115), (255, 255, 213)]
OUTLINE = GREY[1]

BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]

# geometry (pixels, image coordinates; dx = distance from the axis, negative on the left)
SPH_CY, SPH_RX, SPH_RY = 70, 78, 70          # the hull sphere
RING_CY, RING_RY, RING_RX, RING_T = 80, 22, 118, 7   # the ring's centre line and half thickness
WIN_CY, WIN_RX, WIN_RY = 26, 24, 12          # cockpit dome
CREST_CY, CREST_R = 56, 9
DOOR_TOP, DOOR_BOT, DOOR_HW = 110, 138, 15   # the doorway (arched top)
RAMP_BOT = 159


def ramp(colours, v, x, y):
    """v in 0..1 over a colour ramp, ordered-dithered between neighbours."""
    v = min(max(v, 0.0), 1.0) * (len(colours) - 1)
    i = int(v)
    if i >= len(colours) - 1:
        return colours[-1]
    return colours[i + 1] if (v - i) * 16 > BAYER[y & 3][x & 3] + 0.5 else colours[i]


def ring_y(dx, front=True):
    t = 1 - (dx / RING_RX) ** 2
    if t < 0:
        return None
    return RING_CY + (1 if front else -1) * RING_RY * math.sqrt(t)


def in_sphere(dx, y):
    return (dx / SPH_RX) ** 2 + ((y - SPH_CY) / SPH_RY) ** 2 <= 1


def fin_poly():
    # the left fin, swept back and down from the ring (x, y)
    return [(62, 70), (40, 74), (14, 92), (2, 120), (8, 130), (22, 118), (40, 100), (64, 94)]


def point_in_poly(x, y, poly):
    inside = False
    j = len(poly) - 1
    for i in range(len(poly)):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def seg_dist(px, py, ax, ay, bx, by):
    vx, vy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * vx + (py - ay) * vy) / (vx * vx + vy * vy)))
    return math.hypot(px - ax - t * vx, py - ay - t * vy), t


def leg(x, y, top, foot, width):
    """A landing leg strut from top to foot with a pad; returns a colour or None."""
    fx, fy = foot
    if ((x - fx) / 11) ** 2 + ((y - fy) / 4.5) ** 2 <= 1:          # the foot pad
        return GREY[5] if y < fy - 1 else GREY[3]
    d, t = seg_dist(x, y, top[0], top[1], fx, fy - 2)
    if d <= width:
        if abs(t - 0.45) < 0.08 and d <= width + 0.5:                # the hydraulic joint
            return GREY[6]
        return GREY[6] if d < width * 0.4 else GREY[4]
    return None


def hull(x, y, dx):
    """The sphere's own surface at (x, y)."""
    nx = dx / SPH_RX
    ny = (y - SPH_CY) / SPH_RY
    nz = math.sqrt(max(0.0, 1 - nx * nx - ny * ny))
    lx, ly, lz = 0.0, -0.62, 0.78                                    # light: above, in front
    lit = max(0.0, nx * lx + ny * ly + nz * lz)
    spec = max(0.0, 1 - math.hypot(nx * 1.6, (ny + 0.55) * 2.2)) ** 3   # a soft glint high on the dome
    v = 0.12 + 0.72 * lit + 0.35 * spec
    c = ramp(GREY[2:], v, x, y)
    # panel lines: latitudes and meridians on the upper hull, rivets along them
    lat = math.sqrt(max(1e-6, 1 - ny * ny))
    u = nx / lat
    line = False
    for k in (-0.62, -0.30):
        if abs(ny - k) * SPH_RY < 0.7:
            line = True
            if abs((u * 8) - round(u * 8)) < 0.06:
                return GREY[8]
    if ny < 0.05 and ny > -0.86:
        for k in (0.36, 0.74):
            if abs(abs(u) - k) * SPH_RX * lat < 0.6:
                line = True
    if line:
        i = GREY.index(c) if c in GREY else 4
        return GREY[max(1, i - 2)]
    # vents on the lower hull, beside the door
    if 0.62 < ny < 0.86 and 34 < abs(dx) < 52 and int(abs(dx)) % 5 in (0, 1):
        return GREY[2]
    return c


def pixel(x, y):
    dx = x + 0.5 - CX
    ax = abs(dx)
    mx = CX - ax - 0.5                       # the same point mirrored onto the left half (fin / leg tests)
    out = None
    # back of the scene: the ring's back arc beside the sphere, the outer legs, the fins
    yb = ring_y(dx, front=False)
    if yb is not None and abs(y - yb) <= RING_T - 2 and not in_sphere(dx, y):
        out = PURPLE[2] if y < yb else PURPLE[1]
    c = leg(mx, y, (52, 108), (22, 150), 3.2)
    if c:
        out = c
    if point_in_poly(mx, y, fin_poly()):
        edge = y < 78 + (62 - mx) * 0.5
        out = ramp(PURPLE[1:6], 0.75 if edge else 0.25 + 0.4 * (mx / 64), x, y)
        if mx < 46 and abs((mx * 0.55 + y) - 112) < 1.1:            # a stripe along the fin
            out = PURPLE[5]
    # the hull
    if in_sphere(dx, y):
        out = hull(x, y, dx)
        # cockpit dome
        wq = (dx / WIN_RX) ** 2 + ((y - WIN_CY) / WIN_RY) ** 2
        if wq <= 1:
            if wq > 0.78:
                out = PURPLE[1]
            else:
                out = ramp(RED[:3], (y - (WIN_CY - WIN_RY)) / (2 * WIN_RY) * 0.9 + 0.05, x, y)
                if 0.36 < wq < 0.52 and y < WIN_CY - 4:             # a highlight arc across the glass
                    out = RED[4] if 0.41 < wq < 0.47 else RED[3]
        # the crest: a red ring with a white face and a purple Saiyan mark
        cq = math.hypot(dx, y - CREST_CY)
        if cq <= CREST_R:
            if cq > CREST_R - 2:
                out = RED[1]
            else:
                out = GREY[8]
                if (ax <= 1.5 and abs(y - CREST_CY) <= 5) or (abs(y - CREST_CY + 1) <= 1 and ax <= 5 and ax >= 2.5):
                    out = PURPLE[1]
                elif abs(y - CREST_CY - 3) <= 0.6 and ax <= 4:
                    out = PURPLE[2]
    # the ring's front arc (over the hull)
    yf = ring_y(dx)
    if yf is not None and abs(y - yf) <= RING_T:
        t = y - yf
        if t < -RING_T + 1 or t > RING_T - 1:
            out = OUTLINE
        elif t < -3:
            out = PURPLE[5] if t < -5 else PURPLE[4]
        elif t < 4:
            out = ramp(PURPLE[1:4], 0.85 - (t + 3) / 8, x, y)
            # lights every 16 px along the ring (symmetric: at 8, 24, 40, ... from the axis)
            k = round((ax - 8) / 16)
            lx = 8 + 16 * k
            if k >= 0 and lx < RING_RX - 6 and math.hypot(ax - lx, t - 0.5) <= 2.2:
                out = LIGHT[1] if math.hypot(ax - lx, t - 0.5) < 1.1 else LIGHT[0]
        else:
            out = PURPLE[0]
    # the doorway: an arch under the ring, lit from inside
    arch_cy = DOOR_TOP + DOOR_HW
    in_door = (DOOR_TOP <= y <= DOOR_BOT and ax <= DOOR_HW and (y >= arch_cy or math.hypot(dx, y - arch_cy) <= DOOR_HW))
    in_frame = (DOOR_TOP - 3 <= y <= DOOR_BOT and ax <= DOOR_HW + 3
                and (y >= arch_cy or math.hypot(dx, y - arch_cy) <= DOOR_HW + 3))
    if in_frame and not in_door:
        out = PURPLE[1] if ax > DOOR_HW + 1.5 or y < arch_cy - DOOR_HW - 1 else PURPLE[3]
    if in_door:
        v = (y - DOOR_TOP) / (DOOR_BOT - DOOR_TOP)
        out = ramp(GLOW, 0.15 + 0.85 * v, x, y)
        if ax > DOOR_HW - 2:
            out = GLOW[0]
        if y < DOOR_TOP + 4 and ax < DOOR_HW - 1:                     # the hatch, slid up into the hull
            out = GREY[3] if y < DOOR_TOP + 3 else GREY[2]
    # the ramp, down to the ground
    if DOOR_BOT < y <= RAMP_BOT:
        t = (y - DOOR_BOT) / (RAMP_BOT - DOOR_BOT)
        hw = DOOR_HW + 1 + 8 * t
        if ax <= hw:
            if ax > hw - 2:
                out = GREY[7] if y % 2 else GREY[2]                     # side rails
            elif (y - DOOR_BOT) % 4 == 0:
                out = GREY[4]                                         # step edges
            else:
                out = ramp([GREY[5], GREY[6], GLOW[4]], 0.9 - 0.8 * t, x, y)
    # the inner legs, in front of the hull
    c = leg(mx, y, (78, 124), (62, 152), 3.0)
    if c and not (DOOR_BOT < y and ax <= DOOR_HW + 10):
        out = c
    return out


def draw():
    """The ship as an RGBA PIL image (transparent around it), outlined."""
    from PIL import Image
    px = [[None] * W for _ in range(H)]
    for y in range(H):
        for x in range(CX):
            c = pixel(x, y)
            px[y][x] = c
            px[y][W - 1 - x] = c
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    for y in range(H):
        for x in range(W):
            c = px[y][x]
            if c is None:
                continue
            edge = any(not (0 <= x + ox < W and 0 <= y + oy < H) or px[y + oy][x + ox] is None
                       for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            im.putpixel((x, y), (OUTLINE if edge else c) + (255,))
    return im


# the doorway's walkable tiles (columns 14-17 = x 112-143, rows 14-19 = y 112-159: the doorway and the ramp)
DOOR_TILES = (14, 14, 4, 6)


def footprint():
    """The ship's collision in 8-px tiles relative to its top-left: {(tx, ty): 0 block / 1 walk}. Every tile
    with 6+ ship pixels blocks, and each column from its top blocked tile to its bottom one (no gaps between
    the ring and the legs: the ship is drawn under the players, so nobody may stand behind any of it); the
    doorway and the ramp stay walkable (the door trigger sits in the doorway)."""
    im = draw()
    a = im.getchannel("A")
    out = {}
    for tx in range(W // 8):
        rows = [ty for ty in range(H // 8)
                if sum(1 for p in range(64) if a.getpixel((tx * 8 + p % 8, ty * 8 + p // 8)) > 127) >= 6]
        for ty in range(min(rows), max(rows) + 1) if rows else ():
            out[(tx, ty)] = 0
    c, r, w, h = DOOR_TILES
    for tx in range(c, c + w):
        for ty in range(r, r + h):
            out[(tx, ty)] = 1
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    im = draw()
    im.save(sys.argv[1])
    if "--x4" in sys.argv:
        from PIL import Image
        bg = Image.new("RGBA", im.size, (96, 160, 64, 255))
        bg.alpha_composite(im)
        bg.resize((W * 4, H * 4), Image.NEAREST).save(sys.argv[sys.argv.index("--x4") + 1])


if __name__ == "__main__":
    main()
