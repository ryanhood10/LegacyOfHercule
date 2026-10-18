"""Build the co-op ROM: pad the original to 16 MiB, drop the compiled code at 0x800000, rewrite the
vtable slots that redirect into it, and patch function entries with long jumps.

usage: patch_rom.py ORIG.gba coop.bin coop.sym OUT.gba [--debug-menu] [--wide] [--data-patch FILE.json ...]

--data-patch FILE.json applies generic data records to the output ROM (after the code and hooks).
The file is a JSON list; each record is an object with a file offset and one of:
    {"offset": "0x1D9548", "bytes": "32003200..."}        raw bytes (hex string)
    {"offset": "0x8024",   "word":  "0x08900000"}         one little-endian u32
    {"offset": "0x8024",   "word":  "sym:ability_table"}  u32 = that symbol's address from coop.sym
    {"offset": "0x8024",   "word":  "fn:hercule_taunt"}   u32 = symbol address | 1 (Thumb function pointer)
An optional "expect": "5b" (hex) aborts the build if the bytes there differ, and "note" is printed.
Records are applied in order; tools/romdata.py and tools/script_dump.py generate these files.
"""
import json
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

# --wide: draw slot (4) of every class whose draw is the base sprite emitter 0x08007773, redirected
# to sprite_emit_hook (adds OAM x bit 9 / y bit 8 for the wide-view mGBA fork)
SPRITE_VTABLES = [0x0802489C, 0x08024E84, 0x080252A4, 0x08025564, 0x080255CC, 0x08025604, 0x0802564C,
                  0x08025770, 0x080257A0, 0x080257D0, 0x0802593C, 0x08025C30, 0x08025F28]

# (function address, symbol): the entry becomes  ldr r3,[pc,#n]; bx r3; (nop;) .word symbol|1
# 8 bytes when addr % 4 == 0; 10 bytes (with a nop before the word) when addr % 4 == 2.
ENTRY_JUMPS = [
    (0x0801EEFC, "input_poll_hook"),         # Input_Poll
    (0x080069A4, "entity_list_update_hook"), # EntityList_Update
    (0x0800E4D2, "exp_award_hook"),          # enemy death -> EXP event
    (0x0800E77E, "enemy_spawn_hook"),        # MapScript_CreateEnemy (map loader record handler)
    (0x0800F910, "damage_calc_hook"),        # damage calculation (defender, hit)
    (0x0801B332, "ai_speed8_create_hook"),   # 8-direction chase AI module constructor (speed param)
    (0x0801B15E, "ai_speed4_create_hook"),   # axis-aligned approach AI module constructor
    (0x080104C0, "bossbar_create_hook"),     # BossHealthBar_Create (DrawBossHealthBar opcodes)
]

# (file offset, old word, new word): Arena_Init's heap size in 4-byte words. The heap otherwise runs
# from 0x02000800 to 0x0203FFF8, over the co-op state at the top of EWRAM; keep the top 1.25 KiB
# (0x0203FB00 co-op difficulty state, 0x0203FE00 player-2 vtable, 0x0203FF00 pad words + CoopState).
# (old literal value, symbol, add, [file offsets]): every listed pool word must hold the old value and
# becomes symbol+add. Used to move ROM tables that have no spare room into the co-op code area:
# Character_DisplayData (22 rows -> 23, the 18 literals from the reference notes) and
# g_CharacterSpriteIndex (the one literal loads table+7 words).
LITERAL_RELOCATIONS = [
    (0x081D967C, "display_rows", 0, [0x1954, 0x1B3C, 0x3B30, 0x414C, 0x42F0, 0x854C, 0x886C, 0x8B04, 0xA77C, 0xAA04,
                                     0xB050, 0xC804, 0x10F18, 0x112BC, 0x13574, 0x14FF4, 0x26458, 0x26474]),
    (0x083B4E90, "sprite_index", 0x1C, [0x959C]),
]
# (file offset, old byte, new byte): CharacterData_FindBySprite's row limit (CMP R2,#imm8) 22 -> 23
BYTE_PATCHES = [
    (0x42D6, 0x16, 0x17),
]

WORD_PATCHES = [
    (0x01F20C, 0x0000FDFE, 0x0000FCC0),
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


def apply_data_patch(rom, path, syms):
    """Apply the (offset, bytes) records of a JSON data patch file (format: see the header)."""
    for rec in json.load(open(path)):
        off = int(rec["offset"], 0) if isinstance(rec["offset"], str) else rec["offset"]
        if "bytes" in rec:
            data = bytes.fromhex(rec["bytes"])
        else:
            w = rec["word"]
            if isinstance(w, str) and w.startswith("sym:"):
                w = syms[w[4:]]
            elif isinstance(w, str) and w.startswith("fn:"):
                w = syms[w[3:]] | 1
            elif isinstance(w, str):
                w = int(w, 0)
            data = struct.pack("<I", w)
        if "expect" in rec:
            exp = bytes.fromhex(rec["expect"])
            if rom[off:off + len(exp)] != exp:
                sys.exit(f"data patch {path} at {off:#x}: expected {exp.hex()}, found {rom[off:off + len(exp)].hex()}")
        if off + len(data) > len(rom):
            sys.exit(f"data patch {path} at {off:#x}: {len(data)} bytes run past the ROM")
        rom[off:off + len(data)] = data
        print(f"data   {ROM_BASE + off:08x} {len(data):5} bytes  {rec.get('note', '')}")


def main():
    orig, blob, symfile, out = sys.argv[1:5]
    debug_menu = "--debug-menu" in sys.argv[5:]
    wide = "--wide" in sys.argv[5:]
    data_patches = [sys.argv[i + 1] for i, a in enumerate(sys.argv) if a == "--data-patch"]
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

    for off, old, new in WORD_PATCHES:
        cur = struct.unpack_from("<I", rom, off)[0]
        if cur != old:
            sys.exit(f"word patch at {off:#x}: expected {old:08x}, found {cur:08x}")
        struct.pack_into("<I", rom, off, new)
        print(f"word   {ROM_BASE + off:08x} {old:08x} -> {new:08x}")

    for old, name, add, offs in LITERAL_RELOCATIONS:
        new = (syms[name] + add) & 0xFFFFFFFF
        for off in offs:
            cur = struct.unpack_from("<I", rom, off)[0]
            if cur != old:
                sys.exit(f"relocation at {off:#x}: expected {old:08x}, found {cur:08x}")
            struct.pack_into("<I", rom, off, new)
        print(f"reloc  {old:08x} -> {new:08x} ({name}+{add:#x}) at {len(offs)} literals")
    for off, old, new in BYTE_PATCHES:
        if rom[off] != old:
            sys.exit(f"byte patch at {off:#x}: expected {old:02x}, found {rom[off]:02x}")
        rom[off] = new
        print(f"byte   {ROM_BASE + off:08x} {old:02x} -> {new:02x}")

    hooks = list(VTABLE_HOOKS)
    if wide:
        hooks += [(vt, 4, "sprite_emit_hook") for vt in SPRITE_VTABLES]
    for vtable, slot, name in hooks:
        target = syms[name] | 1
        rel = (target - vtable) & 0xFFFFFFFF
        off = vtable - ROM_BASE + slot * 4
        old = struct.unpack_from("<I", rom, off)[0]
        struct.pack_into("<I", rom, off, rel)
        print(f"vtable {vtable:08x}[{slot}] {old:08x} -> {rel:08x} ({name} @ {target:08x})")

    for addr, name in ENTRY_JUMPS:
        target = syms[name] | 1
        off = addr - ROM_BASE
        if addr % 4 == 0:
            struct.pack_into("<HHI", rom, off, 0x4B00, 0x4718, target)   # ldr r3,[pc,#0]; bx r3; .word
        elif addr % 4 == 2:
            # pc reads as (addr+4)&~3 = addr+2, so [pc,#4] is the word at addr+6
            struct.pack_into("<HHHI", rom, off, 0x4B01, 0x4718, 0x46C0, target)   # ldr r3,[pc,#4]; bx r3; nop; .word
        else:
            sys.exit(f"entry jump at {addr:#x} must be 2-byte aligned")
        print(f"entry  {addr:08x} -> {name} @ {target:08x}")

    for path in data_patches:
        apply_data_patch(rom, path, syms)

    open(out, "wb").write(rom)
    print(f"wrote {out}: {len(rom)} bytes, code {len(code)} bytes at {ROM_BASE + CODE_AT:#x}")


if __name__ == "__main__":
    main()
