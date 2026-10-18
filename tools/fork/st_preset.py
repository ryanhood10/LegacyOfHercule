"""Write the chapter-test story-flag preset (coop.h StoryTest) into a STORY_TEST ROM at file 0xC00000:
Party_Init applies it, so the next debug Map Test starts with these flags. Edits the ROM file in place
(a build/ copy; relaunch the fork afterwards).
usage: python tools/fork/st_preset.py ROM [+FLAG ...] [-FLAG ...]      (no flags = clear the preset)"""
import struct
import sys

PRESET_AT, MAGIC = 0xC00000, 0x53545259
rom, flags = sys.argv[1], sys.argv[2:]
sets = [int(f[1:], 0) for f in flags if f.startswith("+")]
clears = [int(f[1:], 0) for f in flags if f.startswith("-")]
if len(sets) > 16 or len(clears) > 8:
    sys.exit("at most 16 flags to set and 8 to clear")
data = struct.pack("<I16H8H", MAGIC if flags else 0, *(sets + [0xFFFF] * (16 - len(sets))),
                   *(clears + [0xFFFF] * (8 - len(clears))))
with open(rom, "r+b") as f:
    f.seek(PRESET_AT)
    f.write(data)
print(f"{rom}: preset set {sets} clear {clears}" if flags else f"{rom}: preset cleared")
