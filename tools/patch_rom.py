"""Build the co-op ROM: pad the original to 16 MiB, drop the compiled code at 0x800000, rewrite the
vtable slots that redirect into it, and patch function entries with long jumps.

usage: patch_rom.py ORIG.gba coop.bin coop.sym OUT.gba
"""
import struct
import sys

ROM_BASE = 0x08000000
CODE_AT = 0x00800000
ROM_SIZE = 0x01000000

# (vtable address, slot, symbol): slot word becomes (symbol | 1) - vtable
VTABLE_HOOKS = [
    (0x08025A84, 2, "idle_update_hook"),   # idle control state
    (0x080256C4, 2, "walk_update"),        # walk control state
    (0x080258DC, 2, "diag_update"),        # diagonal / free movement control state
    (0x08025E20, 2, "dash_update_hook"),   # dash / transformation charge control state
    (0x080251F4, 2, "ctrl_8277_hook"),     # control state reading the d-pad
    (0x08025D70, 5, "ctrl_85ad_hook"),     # control state reading A
    (0x08025A6C, 0, "camera_follow_hook"), # camera-follow object update
    (0x0802408C, 0, "hud_destroy_hook"),   # HUD object
    (0x0802408C, 2, "hud_update_hook"),
    (0x0802408C, 4, "hud_draw_hook"),
]

# (function address, symbol): first 8 bytes become  ldr r3,[pc]; bx r3; .word symbol|1
# The address must be such that (addr + 4) & ~3 == addr + 4, i.e. addr % 4 == 0.
ENTRY_JUMPS = [
    (0x0801EEFC, "input_poll_hook"),         # Input_Poll
    (0x080069A4, "entity_list_update_hook"), # EntityList_Update
]

# Optional: title screen idles into the leftover debug menu (Map Test loads any map with a level-40
# party). From the Arefu/Legacy engine notes: file offset 0x574, BL GameIntroVideo_Create -> BL DebugMenu_Create.
DEBUG_MENU_PATCH = (0x574, bytes.fromhex("1EF0E2FB"), bytes.fromhex("05F09CF8"))


def load_symbols(path):
    syms = {}
    for line in open(path):
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    return syms


def main():
    orig, blob, symfile, out = sys.argv[1:5]
    debug_menu = "--debug-menu" in sys.argv[5:]
    rom = bytearray(open(orig, "rb").read())
    if debug_menu:
        off, old, new = DEBUG_MENU_PATCH
        if rom[off:off + 4] != old:
            sys.exit(f"debug-menu patch: unexpected bytes {rom[off:off + 4].hex()} at {off:#x}")
        rom[off:off + 4] = new
        print("debug menu enabled (idle 30 s on the title screen)")
    code = open(blob, "rb").read()
    syms = load_symbols(symfile)

    if len(rom) > CODE_AT:
        sys.exit(f"original ROM is larger than {CODE_AT:#x}")
    rom += b"\xff" * (ROM_SIZE - len(rom))
    rom[CODE_AT:CODE_AT + len(code)] = code

    for vtable, slot, name in VTABLE_HOOKS:
        target = syms[name] | 1
        rel = (target - vtable) & 0xFFFFFFFF
        off = vtable - ROM_BASE + slot * 4
        old = struct.unpack_from("<I", rom, off)[0]
        struct.pack_into("<I", rom, off, rel)
        print(f"vtable {vtable:08x}[{slot}] {old:08x} -> {rel:08x} ({name} @ {target:08x})")

    for addr, name in ENTRY_JUMPS:
        if addr % 4:
            sys.exit(f"entry jump at {addr:#x} must be 4-byte aligned")
        target = syms[name] | 1
        off = addr - ROM_BASE
        struct.pack_into("<HHI", rom, off, 0x4B00, 0x4718, target)   # ldr r3,[pc,#0]; bx r3; .word
        print(f"entry  {addr:08x} -> {name} @ {target:08x}")

    open(out, "wb").write(rom)
    print(f"wrote {out}: {len(rom)} bytes, code {len(code)} bytes at {ROM_BASE + CODE_AT:#x}")


if __name__ == "__main__":
    main()
