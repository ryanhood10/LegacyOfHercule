r"""Is a save file safe for the Time Chamber's flags (docs/time-chamber.md 12, docs/testing.md "Time Chamber")?
  python -I tools/headless/chamber_save.py check SAVE.sav [SAVE2.sav ...]        read only: each slot's checksum, map, flags 504-535
  python tools/headless/chamber_save.py load ROM SAVE.sav --out DIR [--base ROM2]  a COPY of SAVE.sav into DIR, then a real load of each
                                                                                 slot in that ROM: flags 504-535 clear and the party block
                                                                                 as the file's; --base: the block as ROM2's load gives it
Reads a 9-slot build's 8 KiB EEPROM file. The file holds each 8 bytes reversed (the chip's bit order, as mGBA keeps it); a slot is at
0x10 + n * 0x1C0: PartyState (0x1B8 bytes), its checksum (Adler-32, 0x0802153E), padding. Prints FAIL on a miss, exit 1. A missing
SAVE.sav in `load` prints "skipped" and exits 0 (a worktree without Ryan's build/play).
What a load changes on purpose in a save from before those fixes (LOAD_FIXES below, all older than the Time Chamber): hercule_lock
(Hercule's entry), krillin_lift and flag 462 (Krillin's), the slot 8 row. (`cartout` does not work on this runner's save type; the
load writes the chip only for an eight-slot save, party_save.c `store.migrated`.)"""
import os
import re
import shutil
import subprocess
import sys
import zlib

STRIDE, BLOCK, FLAGS = 0x1C0, 0x1B8, 0x108
HERE = os.path.dirname(os.path.abspath(__file__))


def unswap(raw):
    return b"".join(raw[i:i + 8][::-1] for i in range(0, len(raw), 8))


def slots(path):
    d = unswap(open(path, "rb").read())
    for n in range(3):
        b = d[0x10 + n * STRIDE:0x10 + (n + 1) * STRIDE]
        yield n, b, zlib.adler32(b[:BLOCK]) == int.from_bytes(b[BLOCK:BLOCK + 4], "little")


def check(paths):
    bad = 0
    for path in paths:
        print(path)
        for n, b, good in slots(path):
            ev = b[FLAGS:FLAGS + 0x66]
            hi = ev[63:67]
            flags = [i for i in range(816) if ev[i >> 3] >> (i & 7) & 1]
            print(f" slot {n}: checksum {'good' if good else 'bad (empty)'}, active {b[0]}, saved {b[1] & 1}, map {b[2]}/{b[3]}, "
                  f"flags 504-535 {hi.hex()}, flags set {len(flags)}, above 503: {[i for i in flags if i > 503]}")
            if good and any(hi):
                print(f"FAIL slot {n}: flags 504-535 are not clear")
                bad = 1
    return bad


def load(rom, save, out, base_rom=None):
    if not os.path.exists(save):
        print(f"chamber_save: {save} is not here: skipped")
        return 0
    os.makedirs(out, exist_ok=True)
    copy = os.path.abspath(os.path.join(out, "chamber_save_copy.sav"))
    shutil.copyfile(save, copy)                    # never the original
    before = open(save, "rb").read()
    wsl = "/mnt/" + copy[0].lower() + copy[2:].replace("\\", "/")
    bad = 0
    for n, b, good in slots(copy):
        if not good:
            print(f"slot {n}: empty")
            continue
        downs = "key 0x80 6\nframes 30\n" * n
        script = f"""p2 off
cart {wsl}
frames 900
key 8 6
frames 120
key 1 6
frames 300
{downs}key 1 6
frames 300
key 1 6
frames 300
key 1 6
frames 400
map
peek PARTY 110
"""
        path = os.path.join(out, f"chamber_save_{n}.txt")
        open(path, "w", newline="\n").write(script)
        run = subprocess.run([sys.executable, os.path.join(HERE, "st_test.py"), path, rom, "--out", out],
                             capture_output=True, text=True)
        words = []
        for ln in run.stdout.splitlines():
            m = re.match(r"^0203f[0-9a-f]{3}: (.*)$", ln)
            if m:
                words += [int(w, 16) for w in m.group(1).split()]
            if ln.startswith("map zone"):
                print(f"slot {n}: {ln}")
        got = b"".join(w.to_bytes(4, "little") for w in words)[:BLOCK]
        if len(got) != BLOCK:
            print(f"FAIL slot {n}: the party block was not read ({len(got)} bytes)\n{run.stdout[-500:]}")
            bad = 1
            continue
        hi = got[FLAGS + 63:FLAGS + 67]
        diff = [i for i in range(BLOCK) if got[i] != b[i]]
        other = [i for i in diff if not (0x98 <= i < 0xD0 or 0xEC <= i < 0x108 or i == FLAGS + 462 // 8)]   # LOAD_FIXES
        print(f"slot {n}: flags 504-535 after the load {hi.hex()}, bytes that differ from the file: {[hex(i) for i in diff]}, "
              f"outside what a load fixes on purpose: {[hex(i) for i in other]}")
        if any(hi):
            print(f"FAIL slot {n}: flags 504-535 are not clear after the load")
            bad = 1
        if other:
            print(f"FAIL slot {n}: the load changed the block at {[hex(i) for i in other]}")
            bad = 1
        if base_rom:
            run = subprocess.run([sys.executable, os.path.join(HERE, "st_test.py"), path, base_rom, "--out", out],
                                 capture_output=True, text=True)
            words = []
            for ln in run.stdout.splitlines():
                m = re.match(r"^0203f[0-9a-f]{3}: (.*)$", ln)
                if m:
                    words += [int(w, 16) for w in m.group(1).split()]
            base = b"".join(w.to_bytes(4, "little") for w in words)[:BLOCK]
            same = base == got
            print(f"slot {n}: the same load in the base ROM gives {'the same block' if same else 'ANOTHER block'}")
            if not same:
                print(f"FAIL slot {n}: differs from the base ROM at {[hex(i) for i in range(BLOCK) if base[i:i+1] != got[i:i+1]]}")
                bad = 1
    if open(save, "rb").read() != before:
        print("FAIL the original save changed")
        bad = 1
    print("chamber_save: FAIL" if bad else "chamber_save: ok, every slot loads unchanged with flags 504-535 clear")
    return bad


def main():
    a = sys.argv[1:]
    if a[0] == "check":
        sys.exit(check(a[1:]))
    out = "."
    if "--out" in a:
        i = a.index("--out")
        out = a[i + 1]
        del a[i:i + 2]
    base = None
    if "--base" in a:
        i = a.index("--base")
        base = a[i + 1]
        del a[i:i + 2]
    sys.exit(load(a[1], a[2], out, base))


if __name__ == "__main__":
    main()
