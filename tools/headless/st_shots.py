"""Convert the headless runner's PPM screenshots to PNG and tile them into one contact sheet.
usage: python tools/headless/st_shots.py SHEET.png SHOT.ppm [SHOT.ppm ...] [--cols 3] [--scale 1]"""
import sys
from PIL import Image

args = sys.argv[1:]
cols, scale = 3, 1
if "--cols" in args:
    i = args.index("--cols"); cols = int(args[i + 1]); del args[i:i + 2]
if "--scale" in args:
    i = args.index("--scale"); scale = int(args[i + 1]); del args[i:i + 2]
out, files = args[0], args[1:]
ims = [Image.open(f).convert("RGB") for f in files]
for f, im in zip(files, ims):
    if f.endswith(".ppm"):
        im.save(f[:-4] + ".png")
w, h = ims[0].size
rows = (len(ims) + cols - 1) // cols
sheet = Image.new("RGB", (cols * w * scale, rows * h * scale), (40, 40, 40))
for i, im in enumerate(ims):
    sheet.paste(im.resize((w * scale, h * scale), Image.NEAREST), ((i % cols) * w * scale, (i // cols) * h * scale))
sheet.save(out)
print(f"{out}: {len(ims)} shots")
