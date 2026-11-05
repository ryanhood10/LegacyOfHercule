"""Placeholder entrances for the Time Chamber's open area (docs/time-chamber.md phase 5 draws the real ones): our own art drawn
by code, no ROM data. draw(NAME) is a 96 x 80 px flat building in the room's colour with a dark doorway (x 32-63, y 48-79),
painted by mapbuild's {"op": "png", "draw": "chamberart", "arg": NAME, "at": [col, row]}. They use few tile slots: the map's budget
leaves room for six real entrances of about 96 x 80 (each at most about 100 slots).
"""
from PIL import Image, ImageDraw

W, H = 96, 80
COLOURS = {                  # roof, wall, edge (BG palette colours, the builder takes the nearest)
    "namek": ((90, 189, 90), (57, 123, 57), (0, 57, 0)),
    "frieza": ((189, 115, 255), (123, 0, 255), (57, 0, 123)),
    "saiyan": ((156, 156, 156), (90, 90, 90), (41, 41, 41)),
    "pirate": ((205, 98, 0), (123, 57, 0), (65, 41, 0)),
    "lab": ((123, 148, 189), (65, 90, 123), (41, 57, 90)),
    "dojo": ((238, 180, 115), (189, 0, 0), (123, 0, 0)),
}


def draw(name="dojo"):
    roof, wall, edge = COLOURS[name]
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle((0, 24, W - 1, H - 1), fill=wall, outline=edge)
    d.rectangle((0, 0, W - 1, 24), fill=roof, outline=edge)
    d.rectangle((32, 48, 63, H - 1), fill=(0, 0, 0), outline=edge)
    return im


if __name__ == "__main__":
    import sys
    sheet = Image.new("RGBA", (W * len(COLOURS), H), (255, 255, 255, 255))
    for i, k in enumerate(COLOURS):
        pic = draw(k)
        sheet.alpha_composite(pic, (i * W, 0))
    sheet.save(sys.argv[1])
