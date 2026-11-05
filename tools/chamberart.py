"""The Time Chamber's six entrances on the open map (docs/time-chamber.md phase 5): our own art drawn by code, no ROM
data, in the family of tools/shipart.py. draw(NAME) is a 96 x 80 px RGBA picture (transparent around the building),
painted by mapbuild's {"op": "png", "draw": "chamberart", "arg": NAME, "at": [col, row]}:

  namek   a Namekian dome house: a white dome with a knob on top, green bands, round windows, an arched door
  frieza  Frieza's round saucer: a pale dome with a band of purple windows, a rim of lights, legs, a ramp down
  saiyan  Paragus' round ship (shipart.py's design, redrawn small): grey sphere, red cockpit, purple ring and fins
  pirate  a pirate airship docked side-on: a red and black balloon with a skull, a planked hull, portholes, a gangway
  lab     Dr. Gero's bunker: a rock face, a concrete front, a heavy steel door slid open, hazard chevrons, red lights
  dojo    Roshi's dojo: a grey tiled roof with curled eaves and a gable, red pillars, a sign board, a curtained door

Every picture is left-right mirror-symmetric about x = 48 (the right half is the left half mirrored, so the map
builder reuses every left tile h-flipped: at most 60 slots each), uses exact BG palette colours (so the builder's
nearest-colour match keeps them), has a dark outline against the white void and a light ground shadow, and keeps its
doorway dark where the map's walk rect expects it: x 32-63, y 56-79 (tiles 4-7, rows 7-9).

usage: chamberart.py OUT.png [--x4 OUT_x4.png]     the six on the white floor (and a 4x copy)
"""
import math
import sys

W, H = 96, 80
CX = 48                       # mirror axis between pixel 47 and 48
NAMES = ("namek", "frieza", "saiyan", "pirate", "lab", "dojo")

# BG palette colours (exact entries)
BLACK = (0, 0, 0)
GREY = [(0, 0, 0), (41, 41, 41), (65, 65, 65), (90, 90, 90), (123, 123, 123), (156, 156, 156),
        (189, 189, 189), (222, 222, 222), (255, 255, 255)]
PURPLE = [(57, 0, 123), (90, 0, 189), (123, 0, 255), (156, 57, 255), (189, 115, 255), (222, 180, 255),
          (238, 213, 255)]
RED = [(123, 0, 0), (189, 0, 0), (255, 0, 0), (255, 90, 90), (255, 148, 148), (255, 213, 213)]
GLOW = [(123, 57, 0), (205, 98, 0), (238, 115, 0), (255, 172, 90), (255, 205, 148), (255, 238, 213)]
LIGHT = [(255, 255, 115), (255, 255, 213)]
YELLOW = [(123, 123, 0), (156, 156, 0), (189, 189, 0), (222, 222, 0), (255, 255, 0)]
WOOD = [(98, 57, 32), (139, 82, 41), (189, 106, 41), (205, 131, 82), (222, 164, 131), (246, 230, 222)]
TAN = [(98, 74, 65), (131, 106, 82), (164, 131, 106), (205, 172, 148), (222, 197, 180), (246, 238, 230)]
GREEN = [(24, 65, 0), (0, 123, 0), (0, 156, 0), (0, 189, 0), (90, 255, 90), (148, 255, 148)]
BLUE = [(0, 57, 123), (0, 82, 172), (24, 139, 255), (90, 172, 255), (148, 205, 255), (213, 238, 255)]
LILAC = (213, 213, 255)
OUTLINE = GREY[1]
SHADOW = GREY[7]

BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def ramp(colours, v, x, y):
    """v in 0..1 over a colour ramp, ordered-dithered between neighbours."""
    v = min(max(v, 0.0), 1.0) * (len(colours) - 1)
    i = int(v)
    if i >= len(colours) - 1:
        return colours[-1]
    return colours[i + 1] if (v - i) * 16 > BAYER[y & 3][x & 3] + 0.5 else colours[i]


def ell(ax, yc, cy, rx, ry):
    """<= 1 inside the ellipse centred on the axis at height cy."""
    return (ax / rx) ** 2 + ((yc - cy) / ry) ** 2


def lit(ax, yc, cy, rx, ry, light=(-0.62, 0.78)):
    """Brightness 0..1 of an ellipsoid's surface lit from above and in front."""
    nx, ny = ax / rx, (yc - cy) / ry
    nz = math.sqrt(max(0.0, 1 - nx * nx - ny * ny))
    return max(0.0, ny * light[0] + nz * light[1])


def seg_dist(px, py, ax, ay, bx, by):
    vx, vy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * vx + (py - ay) * vy) / (vx * vx + vy * vy)))
    return math.hypot(px - ax - t * vx, py - ay - t * vy), t


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


def arch(ax, yc, top, hw):
    """Inside a round-topped opening of half width hw whose crown is at y = top."""
    cy = top + hw
    return yc >= top and ax <= hw and (yc >= cy or math.hypot(ax, yc - cy) <= hw)


# -- the dojo -----------------------------------------------------------------------------------------------------

def dojo(x, y, ax):
    yc = y + 0.5
    # the stone base, with a darker step up into the door
    if 71 <= y and ax <= 43:
        if ax < 15.5:
            if y in (71, 75):
                return GREY[4]
            return GREY[2] if y < 75 else GREY[3]
        if y == 71:
            return GREY[7]
        if y == 79 or (y - 72) % 4 == 3:
            return GREY[4]
        if (int(ax) + 6 * ((y - 72) // 4)) % 12 == 0:
            return GREY[4]
        return ramp(GREY[5:8], 0.9 - (y - 72) / 8, x, y)
    # the roof: a ridge, a gable in the middle, grey tiles down to eaves that curl up at the tips
    eave = 31.5 - 9 * (ax / 47.5) ** 3
    top = 6.0 if ax <= 24 else 6 + 14.5 * ((ax - 24) / 23.5) ** 1.3
    if ax <= 26.5 and 2.5 <= yc <= 6.5 or (22.5 <= ax <= 26.5 and 3.4 - (ax - 22.5) * 0.8 <= yc <= 6.5):
        if yc < 3.5 or (ax > 22.5 and yc < 4.5 - (ax - 22.5) * 0.8):
            return GREY[5]                                     # the ridge's top and the end tiles' curl
        return GREY[2] if ax > 22.5 else GREY[3]
    if top <= yc <= eave:
        if 7 <= yc <= 19.5 and ax <= (yc - 7) * 1.45:          # the gable
            if ax > (yc - 7) * 1.45 - 2 or yc > 18.5:
                return WOOD[0]
            if yc > 17 and ax <= 14:
                return WOOD[2] if int(ax) % 2 else WOOD[1]    # the gable's little rafters
            return WOOD[1] if int(ax) % 3 == 2 else WOOD[3]
        if yc > eave - 2.2:                                   # the tile ends along the eave
            return GREY[6] if int(ax) % 4 in (1, 2) else GREY[2]
        k = int(ax) % 4
        if k == 0:
            return GREY[2]
        if k == 2:
            return ramp(GREY[4:7], 0.9 - (yc - top) / 30, x, y)
        return ramp(GREY[3:6], 0.8 - (yc - top) / 30, x, y)
    if eave < yc < 32.5 and ax <= 42:                          # under the eaves: rafter ends, then shade
        if yc < eave + 2:
            return WOOD[1] if int(ax) % 3 == 1 else GREY[1]
        return GREY[1]
    if 32 <= y <= 35 and ax <= 41:                             # the head beam
        return WOOD[2] if y == 32 else (WOOD[0] if y == 35 else WOOD[1])
    if not (36 <= y <= 70 and ax <= 40):
        return None
    # the pillars, red lacquer
    for a0, a1 in ((36.5, 40.5), (15.5, 19.5)):
        if a0 <= ax <= a1:
            u = (ax - a0) / (a1 - a0)
            if y >= 67:
                return GREY[6] if y == 67 else GREY[5]         # the stone plinth
            if u < 0.25 or u > 0.85:
                return RED[0]
            return RED[2] if 0.45 < u < 0.65 else RED[1]
    if ax > 19.5:                                              # the wall between the pillars
        if 23.5 <= ax <= 32.5 and 39 <= y <= 47:               # a barred window
            if ax > 31.5 or ax < 24.5 or y in (39, 47):
                return WOOD[0]
            return WOOD[2] if int(ax) % 3 == 0 else GREY[1]
        if y <= 37:
            return TAN[4]
        if y <= 49:
            return TAN[5]
        if y <= 52:
            return WOOD[0] if y == 52 else WOOD[1]
        if y == 53:
            return WOOD[3]
        return WOOD[1] if int(ax) % 4 == 0 else WOOD[2]
    # over the door: the sign board on plaster
    if y <= 47:
        if ax <= 12.5 and 37 <= y <= 46:
            if ax > 11.5 or y in (37, 46):
                return WOOD[0]
            r = math.hypot(ax, yc - 41.5)
            if r <= 3.6:
                if r > 2.6:
                    return RED[1]
                if ax <= 1 or (y == 41 and ax <= 2.5):
                    return GREY[1]
                return GREY[8]
            if 5.5 <= ax <= 10 and ((y in (39, 43) and ax <= 9.5) or int(ax) == 7 and 39 <= y <= 44
                                    or (y == 41 and ax >= 6.5)):
                return GREY[1]                                 # brush strokes, as lettering
            return WOOD[4]
        if y == 47:
            return WOOD[0]
        return TAN[5] if y > 37 else TAN[4]
    # the doorway: a blue curtain, then dark
    if y <= 55 and ax <= 14.5:
        if y == 48:
            return BLUE[1]
        if y >= 54 and int(ax) in (4, 9):
            return BLACK
        return BLUE[0]
    return BLACK if y > 58 else ramp([BLACK, GREY[1]], (58.5 - yc) / 4, x, y)


# -- the pirate airship ---------------------------------------------------------------------------------------------

def pirate(x, y, ax):
    yc = y + 0.5
    out = None
    # the gas bag: a long red envelope with seams, black caps at its ends, a skull and crossbones in front
    BCY, BRX, BRY = 13.0, 46.0, 12.0
    q = ell(ax, yc, BCY, BRX, BRY)
    if q <= 1:
        v = 0.12 + 0.88 * lit(ax, yc, BCY, BRX, BRY, (-0.6, 0.8))
        nx, ny = ax / BRX, (yc - BCY) / BRY
        u = nx / math.sqrt(max(1e-6, 1 - ny * ny))
        out = ramp(GREY[1:5], v, x, y) if u > 0.86 else ramp(RED[:4], v, x, y)
        if abs(yc - BCY) < 0.6 and ax > 12:
            out = RED[0]                                       # a seam along its length
        for k in (0.3, 0.58, 0.86):
            if abs(u - k) * BRX * math.sqrt(1 - ny * ny) < 0.55:
                out = GREY[1] if k == 0.86 else RED[0]         # gore seams
        # crossbones behind the skull
        dy = yc - 13.5
        if abs(dy) <= 7.5 and abs(ax - abs(dy) * 1.15) <= 1.0 or math.hypot(ax - 9.0, abs(dy) - 7.6) <= 1.6:
            out = GREY[7]
        sk = ell(ax, yc, 12.0, 6.0, 5.5) <= 1 or (ax <= 3.6 and 14 <= yc <= 18.5)
        if sk:
            out = GREY[8] if yc < 13 else GREY[7]
            if math.hypot(ax - 2.6, yc - 12.8) <= 1.6:
                out = GREY[1]                                  # eyes
            elif ax <= 0.6 and 15 <= yc <= 16:
                out = GREY[2]                                  # nose
            elif yc > 17 and int(ax) % 2 == 0:
                out = GREY[3]                                  # teeth
    # rigging from the gas bag down to the hull
    for a_top, a_bot in ((10.0, 12.0), (24.0, 27.0)):
        if 24 <= yc <= 34:
            t = (yc - 24) / 10
            if abs(ax - (a_top + (a_bot - a_top) * t)) <= 0.5:
                out = WOOD[0]
    # an engine pod on a strut under each side, its propeller spinning outboard
    if 31 <= ax <= 33 and 19 <= yc <= 23:
        out = GREY[3]
    if ((ax - 36) / 5.5) ** 2 + ((yc - 24.5) / 2.6) ** 2 <= 1:
        out = GREY[6] if yc < 24 else GREY[4]
    if ((ax - 42.5) / 1.8) ** 2 + ((yc - 24.5) / 5.5) ** 2 <= 1:
        out = GREY[6] if (y + int(ax)) % 3 else GREY[3]
    # the hull, side-on: a deck rail raised at both ends, planks, a gold trim, portholes
    HTOP, HRX, HRY = 34.0, 47.5, 44.0
    deck = HTOP - 7 * (ax / 47.5) ** 3
    if yc >= deck and (yc <= HTOP or ell(ax, yc, HTOP, HRX, HRY) <= 1):
        r = yc - deck
        if r < 1:
            out = WOOD[3]
        elif r < 3:
            out = WOOD[0] if int(ax) % 4 == 3 and r >= 2 else WOOD[1]
        elif 39 <= y <= 40:
            out = YELLOW[4] if y == 39 else YELLOW[2]
        elif 61 <= y <= 63:
            out = RED[1] if y < 63 else RED[0]
        else:
            row = (y - 41) // 4 if y < 61 else (y - 64) // 4 + 5
            ry = (y - 41) % 4 if y < 61 else (y - 64) % 4
            shade = 0.85 - 0.5 * (yc - HTOP) / HRY - 0.25 * (ax / 47.5) ** 2
            if ry == 3 or (int(ax) + row * 7) % 18 == 0:
                out = WOOD[0]
            else:
                out = ramp(WOOD[0:4], shade + (0.12 if ry == 0 else 0), x, y)
        for pa, pr in ((25.5, 3.6), (38.0, 3.4)):              # portholes
            d = math.hypot(ax - pa, yc - 49)
            if d <= pr:
                if d > pr - 1.2:
                    out = YELLOW[3] if yc < 49 else YELLOW[1]
                else:
                    out = BLUE[3] if (yc < 48 and ax < pa) else BLUE[0]
    # landing struts under the hull's curve
    d, t = seg_dist(ax, yc, 34, 66, 39, 77)
    if d <= 1.4 and out is None:
        out = GREY[5] if d < 0.6 else GREY[3]
    if abs(yc - 78) <= 1.2 and abs(ax - 39) <= 4:
        out = GREY[4] if yc > 78 else GREY[5]
    # the gangway door: an arched hatch, dark inside, a plank floor
    if arch(ax, yc, 44, 15.5) and yc > 43:
        if not arch(ax, yc, 46, 13.5):
            out = WOOD[0] if ax > 14.5 or yc < 45 else YELLOW[2]
        elif yc >= 74:
            out = WOOD[0] if y % 2 else GREY[2]
        else:
            out = BLACK if yc > 52 else ramp([BLACK, GREY[1]], (52 - yc) / 6, x, y)
    return out


# -- Frieza's saucer -------------------------------------------------------------------------------------------------

WHITE_RAMP = [GREY[5], GREY[6], LILAC, GREY[8]]


def frieza(x, y, ax):
    yc = y + 0.5
    out = None
    # the legs, behind
    d, t = seg_dist(ax, yc, 31, 56, 38, 76)
    if d <= 1.8:
        out = GREY[6] if d < 0.7 else GREY[4]
    if ell(ax, yc, 77.3, 5.5, 1.8) <= 1:
        out = GREY[5] if yc < 77.3 else GREY[3]
    # the lower hull
    LCY, LRX, LRY = 54.0, 32.0, 15.0
    if ell(ax, yc, LCY, LRX, LRY) <= 1 and yc > 50:
        out = ramp(GREY[3:7], 0.7 - (yc - 50) / 22, x, y)
        if int(ax) in (10, 22) and yc < 66:
            out = GREY[3]
        if 58 <= yc <= 60 and int(ax) % 6 in (1, 2) and ax > 16:
            out = PURPLE[3] if yc < 59 else PURPLE[1]       # lights under the rim
    # the rim: a wide disc, its top seen from a little above
    RCY, RRX, RRY = 46.0, 47.5, 10.5
    if ell(ax, yc, RCY, RRX, RRY) <= 1:
        front = RCY + RRY * math.sqrt(max(0.0, 1 - (ax / RRX) ** 2))
        e = front - yc
        if e < 3.5:                                         # its edge band
            out = PURPLE[1] if e < 1 else GREY[6]
            if 1 <= e < 3.5 and ((ax + 4) % 8) < 1.6 and ax < 44:
                out = LIGHT[1] if 2 <= e < 3 else LIGHT[0]
        else:
            out = ramp(WHITE_RAMP, 0.95 - 0.3 * (ax / RRX) ** 2 - 0.2 * (yc - RCY + RRY) / RRY, x, y)
    # the dome
    DCY, DRX, DRY = 46.0, 36.0, 39.0
    if yc < 46.5 and ell(ax, yc, DCY, DRX, DRY) <= 1:
        out = ramp(WHITE_RAMP, 0.05 + 1.1 * lit(ax, yc, DCY, DRX, DRY), x, y)
        if abs(yc - 37.5) < 0.6 or abs(yc - 21) < 0.6:
            out = GREY[5]                                   # panel seams
        # the band of round windows
        hw = DRX * math.sqrt(1 - ((29 - DCY) / DRY) ** 2)
        for th in (0.0, 0.42, 0.85, 1.25):
            wa = hw * math.sin(th)
            wr = 3.3 * math.cos(th) + 0.4
            q = ((ax - wa) / wr) ** 2 + ((yc - 29) / 3.4) ** 2
            if q <= 1:
                out = PURPLE[3] if (yc < 28 and q > 0.25) else PURPLE[0]
                if q > 0.75 and yc > 29:
                    out = PURPLE[1]
        # the beacon on top
        if ell(ax, yc, 10.5, 7.5, 4.0) <= 1:
            out = ramp([PURPLE[1], PURPLE[2], PURPLE[4], PURPLE[6]], (11 - yc) / 4 + 0.35, x, y)
    # the hatch and the ramp down
    if arch(ax, yc, 53, 12.5) and yc <= 68:
        if not arch(ax, yc, 55, 10.5):
            out = PURPLE[1] if ax > 11.5 or yc < 54 else PURPLE[3]
        else:
            out = BLACK if yc > 59 else ramp([BLACK, PURPLE[0]], (59 - yc) / 4, x, y)
    if 68 < yc <= 80:
        t = (yc - 68) / 12
        hw = 12.5 + 4 * t
        if ax <= hw:
            if ax > hw - 1.6:
                out = PURPLE[3] if y % 3 else PURPLE[1]
            elif y % 3 == 0:
                out = GREY[1]
            else:
                out = ramp([GREY[2], GREY[3]], 0.3 + t, x, y)
    return out


# -- Paragus' ship (tools/shipart.py, redrawn small) -------------------------------------------------------------------

S_CY, S_RX, S_RY = 34.0, 33.0, 34.0
R_CY, R_RX, R_RY, R_T = 41.0, 47.5, 9.0, 3.5


def saiyan(x, y, ax):
    yc = y + 0.5
    out = None
    insph = ell(ax, yc, S_CY, S_RX, S_RY) <= 1
    # the ring's back arc beside the sphere
    tq = 1 - (ax / R_RX) ** 2
    if tq > 0:
        yb = R_CY - R_RY * math.sqrt(tq)
        if abs(yc - yb) <= R_T - 1.5 and not insph:
            out = PURPLE[2] if yc < yb else PURPLE[1]
    # the outer legs and the swept fins
    mx = CX - ax
    d, t = seg_dist(mx, yc, 20, 50, 8, 73)
    if d <= 1.7:
        out = GREY[6] if d < 0.7 else GREY[4]
    if ell(mx - 8, yc, 74.5, 5, 2) <= 1:
        out = GREY[5] if yc < 74.5 else GREY[3]
    fin = [(22, 40), (13, 43), (5, 51), (1, 62), (4, 66), (9, 58), (15, 51), (23, 48)]
    if point_in_poly(mx, yc, fin):
        out = ramp(PURPLE[1:6], 0.75 if yc < 44 + (22 - mx) * 0.4 else 0.3 + 0.4 * (mx / 24), x, y)
    # the hull
    if insph:
        nx, ny = ax / S_RX, (yc - S_CY) / S_RY
        out = ramp(GREY[2:], 0.12 + 0.72 * lit(ax, yc, S_CY, S_RX, S_RY)
                   + 0.35 * max(0.0, 1 - math.hypot(nx * 1.6, (ny + 0.55) * 2.2)) ** 3, x, y)
        lat = math.sqrt(max(1e-6, 1 - ny * ny))
        u = nx / lat
        if abs(ny + 0.30) * S_RY < 0.6 or (-0.86 < ny < 0.05 and abs(u - 0.55) * S_RX * lat < 0.5):
            i = GREY.index(out) if out in GREY else 4
            out = GREY[max(1, i - 2)]                         # panel lines
        if ell(ax, yc, 12.5, 11.0, 6.5) <= 1:                  # the cockpit dome
            q = ell(ax, yc, 12.5, 11.0, 6.5)
            out = PURPLE[1] if q > 0.72 else ramp(RED[:3], (yc - 6) / 13 * 0.9 + 0.05, x, y)
            if 0.3 < q < 0.5 and yc < 11:
                out = RED[4]
        cq = math.hypot(ax, yc - 26.5)                          # the crest
        if cq <= 4.6:
            out = RED[1] if cq > 3.4 else (PURPLE[1] if ax <= 0.6 or (yc - 26.5 < -0.5 and abs(yc - 25.5) < 0.6 and ax < 2.6)
                                           else GREY[8])
    # the ring's front arc
    if tq > 0:
        yf = R_CY + R_RY * math.sqrt(tq)
        t = yc - yf
        if abs(t) <= R_T:
            if abs(t) > R_T - 1:
                out = OUTLINE
            elif t < -1:
                out = PURPLE[4]
            elif t < 1.5:
                out = PURPLE[2]
                k = round((ax - 4) / 8)
                lx = 4 + 8 * k
                if k >= 0 and lx < R_RX - 3 and math.hypot(ax - lx, t) <= 1.2:
                    out = LIGHT[1] if math.hypot(ax - lx, t) < 0.6 else LIGHT[0]
            else:
                out = PURPLE[0]
    # the door: an arch under the ring, then the ramp
    if arch(ax, yc, 53, 13.5) and yc <= 69:
        if not arch(ax, yc, 55, 11.5):
            out = PURPLE[1] if ax > 12.5 or yc < 54 else PURPLE[3]
        else:
            out = BLACK if yc > 60 else ramp([BLACK, GREY[2]], (60 - yc) / 5, x, y)
            if yc > 66:
                out = ramp([BLACK, GLOW[0]], (yc - 66) / 3, x, y)   # a little light on the floor inside
    if 69 < yc <= 80:
        t = (yc - 69) / 11
        hw = 13 + 4 * t
        if ax <= hw:
            if ax > hw - 1.6:
                out = GREY[6] if y % 2 else GREY[2]
            elif y % 3 == 0:
                out = GREY[2]
            else:
                out = ramp([GREY[3], GREY[4]], 0.8 - t, x, y)
    # the inner legs, in front
    d, t = seg_dist(mx, yc, 30, 60, 24, 74)
    if d <= 1.5 and ax > 18:
        out = GREY[6] if d < 0.6 else GREY[4]
    if ell(mx - 24, yc, 75.5, 4.5, 1.8) <= 1 and ax > 18:
        out = GREY[5] if yc < 75.5 else GREY[3]
    return out


# -- the Namekian house ------------------------------------------------------------------------------------------------

def namek(x, y, ax):
    yc = y + 0.5
    out = None
    DCY, DRX, DRY = 73.0, 45.0, 60.0
    if yc <= DCY + 0.5 and ell(ax, yc, DCY, DRX, DRY) <= 1:
        out = ramp(WHITE_RAMP, 0.05 + 1.1 * lit(ax, yc, DCY, DRX, DRY, (-0.7, 0.71)), x, y)
        hwf = lambda yy: DRX * math.sqrt(max(0.0, 1 - ((yy - DCY) / DRY) ** 2))
        for c, k, th in ((27.0, 3.0, 1.2), (47.0, 5.0, 1.8)):  # green bands, seen from a little above
            yb = c + k * math.sqrt(max(0.0, 1 - (ax / hwf(c)) ** 2))
            if abs(yc - yb) <= th:
                out = GREEN[3] if yc < yb - th / 3 else GREEN[2]
        # round windows, narrower towards the sides
        for wa, wy, wr in ((26.0, 58.5, 4.6), (38.0, 58.5, 3.6), (17.0, 35.0, 3.4), (30.0, 37.0, 2.6)):
            sq = math.sqrt(max(0.05, 1 - (wa / hwf(wy)) ** 2))
            q = ((ax - wa) / (wr * sq)) ** 2 + ((yc - wy) / wr) ** 2
            if q <= 1:
                if q > 0.55:
                    out = GREEN[1] if yc > wy else GREEN[3]
                else:
                    out = BLUE[3] if (yc < wy - 0.8 and q > 0.1) else BLUE[0]
    # the knob on top
    if ell(ax, yc, 9.0, 5.0, 4.5) <= 1:
        out = ramp(WHITE_RAMP, 0.2 + lit(ax, yc, 9.0, 5.0, 4.5), x, y)
    if 12.5 <= yc <= 15 and ax <= 4.5:
        out = GREEN[2] if yc < 14 else GREEN[1]
    # the foundation ring
    if 73 <= y <= 78 and ax <= 46:
        out = GREEN[3] if y == 73 else (GREEN[1] if y < 78 else GREEN[0])
        if 74 <= y <= 76 and int(ax) % 6 == 3:
            out = GREEN[2]
    # the door
    if arch(ax, yc, 44, 15.5):
        if not arch(ax, yc, 46.5, 13.5):
            out = GREEN[3] if yc < 46 or ax < 14.6 else GREEN[1]
            if ax >= 14.6 and yc < 47:
                out = GREEN[2]
        else:
            out = BLACK if yc > 54 else ramp([BLACK, GREY[1]], (54 - yc) / 6, x, y)
    return out


# -- Dr. Gero's bunker ---------------------------------------------------------------------------------------------------

def _hash(a, b):
    h = (a * 374761393 + b * 668265263) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return (h ^ (h >> 16)) & 0xFFFF


ROCK = [GREY[1], TAN[0], TAN[1], TAN[2], TAN[3]]
_SEEDS = []
for _gy in range(-1, 12):
    for _gx in range(0, 7):
        _h = _hash(_gx, _gy)
        _SEEDS.append((_gx * 8 + (_h & 7) - 1.5, _gy * 8 + ((_h >> 3) & 7) - 2, ((_h >> 6) & 255) / 255))


def rock(x, y, ax):
    """Faceted rock (cells around scattered seeds, mirrored): cracks dark, each facet lit from its top."""
    yc = y + 0.5
    best = []
    for sx, sy, b in _SEEDS:
        if abs(sy - yc) < 14:
            best.append(((ax - sx) ** 2 + (yc - sy) ** 2, sx, sy, b))
    best.sort()
    d1, sx, sy, b = best[0]
    d2 = best[1][0]
    if math.sqrt(d2) - math.sqrt(d1) < 1.1:
        return GREY[1] if (math.sqrt(d2) - math.sqrt(d1)) < 0.5 else TAN[0]
    v = 0.3 + 0.35 * b + 0.06 * (sy - yc) + 0.25 * (1 - yc / 80)
    return ramp(ROCK, v, x, y)


def lab(x, y, ax):
    yc = y + 0.5
    out = None
    top = 5 + 17 * (ax / 47.5) ** 2 - 2.5 * abs(math.sin(ax * 0.33)) + 1.5 * abs(math.sin(ax * 0.9))
    if yc >= top:
        out = rock(x, y, ax)
        if yc < top + 1.5:
            out = TAN[3]
    # the concrete front
    if ax <= 27.5 and yc >= 25:
        if yc < 28:
            out = GREY[7] if yc < 26 else GREY[6]
        elif ax > 26.5:
            out = GREY[3]
        else:
            out = ramp(GREY[4:7], 0.75 - (yc - 28) / 70, x, y)
            if (y - 28) % 13 == 12 or int(ax) == 21:
                out = GREY[4]                                  # seams between the slabs
            if 21.5 < ax < 25.5 and 47 <= y <= 61 and y % 2 == 0:
                out = GREY[2]                                  # a vent
    # the beacon on top
    if ax <= 3.5 and 19 <= yc < 25:
        q = ell(ax, yc, 24.5, 3.5, 5.0)
        out = (RED[4] if q < 0.25 and yc < 22 else RED[2] if q < 0.6 else RED[1]) if yc < 24 else GREY[3]
    # the hazard lintel: chevrons, with a red lamp at each end
    if ax <= 20.5 and 30 <= y <= 37:
        if y in (30, 37) or ax > 19.5:
            out = GREY[2]
        else:
            out = YELLOW[4] if ((int(ax) + y) // 3) % 2 else GREY[1]
    r = math.hypot(ax - 24, yc - 33.5)
    if r <= 2.3:
        out = RED[4] if r < 0.9 else RED[2] if r < 1.7 else RED[0]
    # the steel door, slid open into the walls (its edge with hazard stripes), dark inside
    if ax <= 21.5 and y >= 38:
        if y <= 39:
            out = GREY[2] if y == 39 else GREY[3]
        elif ax >= 15.5:
            if ax < 17.5:
                out = YELLOW[4] if (y // 3) % 2 else GREY[1]
            elif ax > 20.5:
                out = GREY[2]
            else:
                out = GREY[3] if y % 5 == 0 else (GREY[6] if ax < 18.5 else GREY[5])
        else:
            out = BLACK if yc > 46 else ramp([BLACK, GREY[1]], (46 - yc) / 6, x, y)
    return out


DRAW = {"namek": namek, "frieza": frieza, "saiyan": saiyan, "pirate": pirate, "lab": lab, "dojo": dojo}
SHADOWS = {                   # the ground shadow's ellipse (centre y, half width, half height)
    "namek": (78.5, 47, 2.5), "frieza": (77.5, 44, 3.0), "saiyan": (77.0, 34, 3.5),
    "pirate": (78.0, 44, 2.5), "lab": (79.0, 47, 1.5), "dojo": (79.0, 47, 1.5),
}


def draw(name="dojo"):
    """Entrance NAME as an RGBA PIL image, 96 x 80, transparent around it, outlined, mirrored about x = 48."""
    from PIL import Image
    f = DRAW[name]
    px = [[None] * W for _ in range(H)]
    for y in range(H):
        for x in range(CX):
            c = f(x, y, CX - x - 0.5)
            px[y][x] = c
            px[y][W - 1 - x] = c
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    scy, srx, sry = SHADOWS[name]
    for y in range(H):
        for x in range(W):
            c = px[y][x]
            if c is None:
                if ell(abs(x + 0.5 - CX), y + 0.5, scy, srx, sry) <= 1:
                    im.putpixel((x, y), SHADOW + (255,))
                continue
            edge = any(not (0 <= x + ox < W and 0 <= y + oy < H) or px[y + oy][x + ox] is None
                       for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if edge and y == H - 1 and c == BLACK:
                edge = False                                   # the doorway stays open at the bottom
            im.putpixel((x, y), (OUTLINE if edge else c) + (255,))
    return im


def sheet(scale=1):
    """The six on the white floor, in a row (scaled by nearest neighbour)."""
    from PIL import Image
    pad = 8
    out = Image.new("RGBA", (pad + (W + pad) * len(NAMES), H + 2 * pad), (252, 252, 252, 255))
    for i, k in enumerate(NAMES):
        out.alpha_composite(draw(k), (pad + i * (W + pad), pad))
    return out.resize((out.width * scale, out.height * scale), Image.NEAREST) if scale > 1 else out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    sheet().save(sys.argv[1])
    if "--x4" in sys.argv:
        sheet(4).save(sys.argv[sys.argv.index("--x4") + 1])


if __name__ == "__main__":
    main()
