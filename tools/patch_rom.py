"""Build the co-op ROM: pad the original to 16 MiB, drop the compiled code at 0x800000, rewrite the
vtable slots that redirect into it, and patch function entries with long jumps.

usage: patch_rom.py ORIG.gba coop.bin coop.sym OUT.gba [--party-slots N] [--debug-menu] [--wide] [--data-patch FILE.json ...]
                    [--maps maps.patch.json]

--data-patch FILE.json applies generic data records to the output ROM (after the code and hooks).
The file is a JSON list; each record is an object with a file offset and one of:
    {"offset": "0x1D9548", "bytes": "32003200..."}        raw bytes (hex string)
    {"offset": "0x8024",   "word":  "0x08900000"}         one little-endian u32
    {"offset": "0x8024",   "word":  "sym:ability_table"}  u32 = that symbol's address from coop.sym
    {"offset": "0x8024",   "word":  "fn:hercule_taunt"}   u32 = symbol address | 1 (Thumb function pointer)
An optional "expect": "5b" (hex) aborts the build if the bytes there differ, and "note" is printed.
Records are applied in order; tools/romdata.py and tools/script_dump.py generate these files.

--maps FILE.json (tools/mapbuild.py build) adds new maps, as the LAST step (after --data-patch, so map
record edits made at the original table offsets are kept): its "records" (chunks, tiles, collision,
variations, ...) are written into free ROM (must be unused 0xFF padding), then the MapEntry table is
relocated to "table_offset": the 327 stock rows as patched so far, followed by the new "rows"; the four
code literals that hold the table address (and &table[3]) and Map_GetCount's constant are repointed.
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import romdata   # noqa: E402  (party table layout shared with tools/romdata.py)

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
    # the beam abilities' charge states read B (src/coop/wrap.c): animations 45, 46, 47, 48, 49, 51
    (0x08025D5C, 2, "masenko_update_hook"),
    (0x080258B4, 2, "bigbang_update_hook"),
    (0x08025E48, 2, "burning_update_hook"),
    (0x08025CB8, 2, "kame_update_hook"),
    (0x08025F70, 2, "sbc_update_hook"),
    (0x08025CCC, 2, "spiritbomb_update_hook"),
    (0x08025A6C, 0, "camera_follow_hook"), # camera-follow object update
    (0x0802408C, 0, "hud_destroy_hook"),   # HUD object
    (0x0802408C, 2, "hud_update_hook"),
    (0x0802408C, 4, "hud_draw_hook"),
    (0x08025604, 9, "player_msg_hook"),     # player message handler: duel hits (duel.c)
    (0x08025604, 16, "p1_take_damage_hook"),# player 1 take-damage: a duel's final blow leaves 1 HP
    # status screen (src/coop/krillin.c): per-slot title, page count, page dispatch; 0x08023B3C is the
    # object 0x08003480 builds, 0x080248CC the pause menu's (0x08004CAC), same methods
    (0x08023B3C, 13, "status_title_hook"),
    (0x08023B3C, 23, "status_pages_hook"),
    (0x08023B3C, 24, "status_draw_hook"),
    (0x080248CC, 13, "status_title_hook"),
    (0x080248CC, 23, "status_pages_hook"),
    (0x080248CC, 24, "status_draw_hook"),
]

# --wide: draw slot (4) of every class whose draw is the base sprite emitter 0x08007773, redirected
# to sprite_emit_hook (adds OAM x bit 9 / y bit 8 for the wide-view mGBA fork)
SPRITE_VTABLES = [0x0802489C, 0x08024E84, 0x080252A4, 0x08025564, 0x080255CC, 0x08025604, 0x0802564C,
                  0x08025770, 0x080257A0, 0x080257D0, 0x0802593C, 0x08025C30, 0x08025F28]

# (function address, symbol): the entry becomes  ldr r3,[pc,#n]; bx r3; (nop;) .word symbol|1
# (functions in R3_ARG_JUMPS take a 4th argument in r3 and jump through r12 instead, see main)
# 8 bytes when addr % 4 == 0; 10 bytes (with a nop before the word) when addr % 4 == 2.
R3_ARG_JUMPS = {0x080104C0}   # BossHealthBar_Create(mem, entity, lo, hi): hi arrives in r3

ENTRY_JUMPS = [
    (0x0801EEFC, "input_poll_hook"),         # Input_Poll
    (0x0800233E, "status_tabs_hook"),        # pause-menu tab strip: tab images by distance, at most 8 tabs (krillin.c)
    (0x080069A4, "entity_list_update_hook"), # EntityList_Update
    (0x0800B4AE, "scene_begin_hook"),       # dialogue sequence start: alone, the lead walks in (companion.c)
    (0x0800B4FC, "scene_end_hook"),         # dialogue sequence end: the stand-in leaves
    (0x0800BDCC, "portrait_get_hook"),      # text portraits: speaker 1 is the story's lead when someone else plays (companion.c)
    (0x0800E4D2, "exp_award_hook"),          # enemy death -> EXP event
    (0x0800E77E, "enemy_spawn_hook"),        # MapScript_CreateEnemy (map loader record handler)
    (0x0800F910, "damage_calc_hook"),        # damage calculation (defender, hit)
    (0x0801B332, "ai_speed8_create_hook"),   # 8-direction chase AI module constructor (speed param)
    (0x0801B15E, "ai_speed4_create_hook"),   # axis-aligned approach AI module constructor
    (0x080104C0, "bossbar_create_hook"),     # BossHealthBar_Create (DrawBossHealthBar opcodes)
    (0x0800DD80, "camera_shake_hook"),       # Camera_Shake: never link the camera into its own chain
    (0x080094B0, "vm_push_name_hook"),       # VM: push the active character's name (dialogue %s)
    (0x080094CC, "vm_push_caps_hook"),       # VM: push the active character's name in capitals
    (0x0800EAD8, "kiblast_spawn_hook"),      # the player ki blast's spawn (Krillin's are recoloured)
]

# (file offset, old word, new word): Arena_Init's heap size in 4-byte words. The heap otherwise runs
# from 0x02000800 to 0x0203FFF8, over the co-op state at the top of EWRAM; keep the top 1.25 KiB
# (0x0203FB00 co-op difficulty state, 0x0203FE00 player-2 vtable, 0x0203FF00 pad words + CoopState).
# The arena hands out [0x02000808, 0x02000808 + 4 * cap): the stock 0xFDFE ends exactly at 0x02040000,
# and the first allocation (the 8 KiB EEPROM mirror, 0x08021454) sits at the very top. The cap now ends
# at 0x0203F000, where coop.ld's partyram region (relocated party block + save-menu slot store) starts:
# (0x0203F000 - 0x02000808) / 4 = 0xF9FE words. (The earlier 0xFCC0 let the mirror's last 8 bytes
# overlap coop2's magic word at 0x0203FB00; nothing wrote them, so it never showed.)
WORD_PATCHES = [
    (0x01F20C, 0x0000FDFE, 0x0000F9FE),
]

# The 27 pointer words that hold the party block's address (g_PartyState, 0x03000E90): the static
# initialiser's literal (file 0x22EE8) and the per-module pointer tables in data. Found by scanning the
# ROM for the value (docs/party-8-slots.md); all become g_party_block so the block can grow.
PARTY_PTR_WORDS = [0x22EE8, 0x1D4F20, 0x1D94D0, 0x1D9534, 0x1D9C10, 0x1D9CA8, 0x1DA8D8, 0x3B50FC, 0x3B526C,
                   0x3B5354, 0x3B5C98, 0x3B5F6C, 0x3EC838, 0x3EC9D0, 0x3ED1A4, 0x3ED1C0, 0x40DAF8, 0x6ADE1C,
                   0x6D60EC, 0x6D7628, 0x6D7640, 0x6DB288, 0x6E3620, 0x6F7E54, 0x6F8078, 0x6F88CC, 0x6FD86C]

# (old literal value, symbol, add, [file offsets]): every listed pool word must hold the old value and
# becomes symbol+add. Used to move ROM tables that have no spare room into the co-op code area:
# Character_DisplayData (22 rows -> 26 since rows 22-25 were added, the 18 literals from the reference notes) and
# g_CharacterSpriteIndex (the one literal loads table+7 words).
LITERAL_RELOCATIONS = [
    (0x081D967C, "display_rows", 0, [0x1954, 0x1B3C, 0x3B30, 0x414C, 0x42F0, 0x854C, 0x886C, 0x8B04, 0xA77C, 0xAA04,
                                     0xB050, 0xC804, 0x10F18, 0x112BC, 0x13574, 0x14FF4, 0x26458, 0x26474]),
    (0x083B4E90, "sprite_index", 0x1C, [0x959C]),
    # UI string table (names, menu labels, technique names): 260 stock entries + per-slot blocks
    # (tools/krillinsprites.py, src/coop/krillin.h UI_*); all 20 literals hold the table's address
    (0x080E2840, "ui_strings", 0, [0x4F8, 0x9B0, 0x1268, 0x1B38, 0x1D88, 0x2524, 0x28A8, 0x3230, 0x3794, 0x3B10,
                                   0x460C, 0x49BC, 0x4C98, 0x95AC, 0xCAE4, 0x10F44, 0x1172C, 0x17A34, 0x1960C, 0x1A688]),
    # portrait module table (word id + 1 = Portrait_Table[id]): Krillin's status portrait appended (id 119)
    (0x083EC9D0, "portrait_module", 0, [0xBE10]),
    # status screen technique pages (0x08003520 / 0x08003684): module table with per-slot icon pairs and
    # string-block bases for PARTY_SLOTS characters (src/coop/krillin.c status_module)
    (0x081D94D0, "status_module", 0, [0x377C]),
    (0x03000E90, "g_party_block", 0, PARTY_PTR_WORDS),
]
# (file offset, old byte, new byte): CharacterData_FindBySprite's row limit (CMP R2,#imm8) 22 -> 29
# (row 22 = Super Saiyan Hercule, row 23 = Krillin, row 24 = old Hercule, row 25 = Android 16, rows 26 / 27 =
# haloed Goku / Super Saiyan Goku, row 28 = Super Saiyan 2 Gohan; tools/spriteclone.py). The search runs
# from row 0, so SetCharacterTransformation(slot, 58) still finds the stock row 4 before row 28.
BYTE_PATCHES = [
    (0x42D6, 0x16, 0x1D),
]


# ---- PARTY_SLOTS (--party-slots N): every ROM site that assumed six party entries -----------------
# The party block lives in EWRAM (PARTY_PTR_WORDS above) and holds N entries; everything after the
# entries moves by (N - 6) * 0x1C. Sites by the kind of patch they need (docs/party-8-slots.md):
#  - C re-implementations with entry jumps (PARTY_ENTRY_JUMPS, src/coop/party.c): Party_Init, the flag
#    accessors and VM flag / counter / item opcodes that add the tail offsets, and the save path.
#  - Thumb imm8 patches (PARTY_COUNT_PATCHES): loop limits "< 6", the VM character-id range, the
#    status-list object size.
#  - Shadow pointer tables (PARTY_SHADOWS): three large constructors reach the flag banks through a
#    private pool word -> per-module pointer table -> party word; the pool word is repointed to a copy
#    of that table whose party word is g_party_block + shift, so their unchanged "+ 0x11A"/"+ 0x12B"
#    instructions land on the moved banks. (pool word file offset, table address, party word offset.)
#  - The default party / growth table: rebuilt with N entries at romdata.PARTY_TABLE_AT, literal 0x3EA0
#    repointed (growth rows stay at literal + 0xA8).
PARTY_ENTRY_JUMPS = [
    (0x08003C24, "party_init_hook"),        # Party_Init(party): defaults + zeroed flag banks
    (0x08003D58, "party_add_exp_hook"),     # Party_AddExp: its module words sat below the moved literal 0x3EA0
    (0x08003E42, "flag_test_hook"),         # Flag_Test(party, id)
    (0x08003E5E, "flag_set_hook"),          # Flag_Set(party, id)
    (0x08003E74, "flag_clear_hook"),        # Flag_Clear(party, id)
    (0x08003F06, "flag_cond_hook"),         # script condition word: flag id | mode << 16, or bytecode pointer
    (0x080095E4, "vm_counter_get_hook"),    # VM opcode: push counters[pop] + live pickups
    (0x080096CA, "vm_counter_dec_hook"),    # VM opcode: counters[pop]--
    (0x080096E8, "vm_counter_sub_hook"),    # VM opcode: counters[idx] -= value
    (0x0800F3F0, "item_use_hook"),          # item use check / counter increment
    (0x0800B00C, "vm_seen_hook"),           # VM opcode: mark a sprite's bestiary entry seen
    # save path (src/coop/party_save.c): new EEPROM layout, slot store outside the menu object
    (0x08004030, "save_game_hook"),         # SaveGame(party, xy): the save pad
    (0x08003F9C, "load_slot_hook"),         # LoadSlot(dst, slot)
    (0x08003FEC, "write_slot_hook"),        # WriteSlot(src, slot)
    (0x08001960, "save_menu_load_hook"),    # save menu: read the three slots
    (0x080019D4, "save_menu_any_hook"),     # any slot saved
    (0x080019F6, "save_menu_saved_hook"),   # slot saved
    (0x08001A0C, "save_menu_location_hook"),   # slot's location name
    (0x08001A26, "save_menu_count_hook"),   # portraits to draw
    (0x08001A44, "save_menu_member_in_party_hook"),
    (0x08001A66, "save_menu_member_selectable_hook"),
    (0x08001A8A, "save_menu_active_pos_hook"),
    (0x08001AB8, "save_menu_member_sprite_hook"),
    (0x08001AE2, "save_menu_set_active_hook"),
    (0x08001B0E, "save_menu_erase_hook"),
    (0x0800180C, "save_menu_start_hook"),   # start / load the chosen slot
    # switch screen (src/coop/switch.c): bigger object, window of six portraits, scrolling
    (0x08010B08, "switch_create_hook"),
    (0x08010B60, "switch_next_hook"),
    (0x08010B9A, "switch_prev_hook"),
    (0x0801110C, "switch_select_hook"),
    (0x080110BC, "switch_arrows_hook"),
    (0x08011172, "switch_update_hook"),
]
# (file offset, stock byte, new byte as a function of n and the number of defined slots d)
PARTY_COUNT_PATCHES = [
    (0x3E18, 0x06, lambda n, d: n),                 # Party_CountInParty 0x08003E00: CMP R1,#6
    (0x3E3A, 0x06, lambda n, d: n),                 # Party_CountFlag 0x08003E1E: CMP R1,#6
    (0x3EFE, 0x06, lambda n, d: n),                 # Party_DetransformOthers 0x08003ED4: CMP R1,#6
    (0x3502, 0x06, lambda n, d: n),                 # status list builder 0x08003480: CMP R0,#6
    (0x3488, 0x71, lambda n, d: 0x71 + (n - 6) * 4),   # ... its object size 0x170 (ADDS R0,#0x71 after #0xFF)
    (0x4E36, 0x71, lambda n, d: 0x71 + (n - 6) * 4),   # status screen 0x08004CAC allocates the same object
    (0xB250, 0x05, lambda n, d: n - 1),             # VM character ids: (id - 2) <= 5 in 0x0800B1E2 (CMP R2,#5)
    (0x5CA6, 0x06, lambda n, d: d),                 # debug party 0x08005C0A fills slots 0..d-1 (CMP R5,#6)
]
# (pool word file offset, table address, party word offset, target): target None = the party block
# shifted by the tail; a symbol name = that object instead (the switch screen's view party).
PARTY_SHADOWS = [
    (0x45F4, 0x081D9C08, 0x8, None),    # 0x08004348 status item list: counters (+0x12B)
    (0x18E3C, 0x086F88C0, 0xC, None),   # 0x08018A38 journal page: seen bit (+0x11A)
    (0x195F4, 0x086FD864, 0x8, None),   # 0x08019204 bestiary list: seen bits (+0x11A)
    (0x10F10, 0x086D763C, 0x4, "switch_view"),   # switch screen 0x08010B08..0x08010BCE (src/coop/switch.c)
    (0x112AC, 0x086D763C, 0x4, "switch_view"),   # ... its update / destroy functions
]
PARTY_SHADOW_AT = romdata.PARTY_TABLE_AT + 0x1000   # shadow tables, 0x200 bytes each
PARTY_SHADOW_SIZE = 0x200


def build_party_table(rom, n):
    """Stock rows 0-5 (and zero rows above) as an N-entry table at PARTY_TABLE_AT; repoint the literal."""
    ent_off, grow_off, literal = romdata.party_table_layout(n)
    size = romdata.ENTRY.size
    rom[ent_off:ent_off + n * size] = bytes(n * size)
    rom[grow_off:grow_off + n * 12] = bytes(n * 12)
    rom[ent_off:ent_off + 6 * size] = rom[romdata.PARTY_OFF:romdata.PARTY_OFF + 6 * size]
    rom[grow_off:grow_off + 6 * 12] = rom[romdata.GROWTH_OFF:romdata.GROWTH_OFF + 6 * 12]
    cur = struct.unpack_from("<I", rom, romdata.PARTY_REF)[0]
    if cur != ROM_BASE + romdata.PARTY_OFF:
        sys.exit(f"party table literal at {romdata.PARTY_REF:#x}: expected {ROM_BASE + romdata.PARTY_OFF:08x}, found {cur:08x}")
    struct.pack_into("<I", rom, romdata.PARTY_REF, literal)
    print(f"party  {n}-entry default table at {ROM_BASE + ent_off:08x}, growth rows at {ROM_BASE + grow_off:08x}, literal {literal:08x}")


def defined_party_slots(rom, n):
    """Entries with a max HP (after the data patch): the debug party fills only those."""
    ent_off = romdata.party_table_layout(n)[0]
    d = 0
    for i in range(n):
        if struct.unpack_from("<H", rom, ent_off + i * romdata.ENTRY.size + 2)[0]:
            d = i + 1
    return d


def apply_party_patches(rom, syms, n):
    """The shadow pointer tables (the entry jumps are applied with ENTRY_JUMPS, the count bytes after
    the data patch by apply_party_count_patches)."""
    shift = (n - 6) * 0x1C
    base = syms["g_party_block"]
    shadows = {}   # (table, word, target) -> file offset of the copy
    for pool, table, word, target in PARTY_SHADOWS:
        cur = struct.unpack_from("<I", rom, pool)[0]
        if cur != table:
            sys.exit(f"shadow pool word at {pool:#x}: expected {table:08x}, found {cur:08x}")
        key = (table, word, target)
        if key not in shadows:
            at = PARTY_SHADOW_AT + len(shadows) * PARTY_SHADOW_SIZE
            rom[at:at + PARTY_SHADOW_SIZE] = rom[table - ROM_BASE:table - ROM_BASE + PARTY_SHADOW_SIZE]
            old_party = struct.unpack_from("<I", rom, at + word)[0]
            if old_party != base:   # the table's party word was relocated above
                sys.exit(f"shadow table {table:08x}+{word:#x}: expected the party block {base:08x}, found {old_party:08x}")
            new = base + shift if target is None else syms[target]
            struct.pack_into("<I", rom, at + word, new)
            shadows[key] = at
            print(f"shadow table {table:08x} -> {ROM_BASE + at:08x} (word +{word:#x} = {new:08x}, {target or 'party + shift'})")
        struct.pack_into("<I", rom, pool, ROM_BASE + shadows[key])
        print(f"shadow pool word {ROM_BASE + pool:08x} -> {ROM_BASE + shadows[key]:08x}")


def apply_party_count_patches(rom, n):
    d = defined_party_slots(rom, n)
    for off, old, fn in PARTY_COUNT_PATCHES:
        new = fn(n, d)
        if rom[off] != old:
            sys.exit(f"party count patch at {off:#x}: expected {old:02x}, found {rom[off]:02x}")
        if not 0 <= new <= 0xFF:
            sys.exit(f"party count patch at {off:#x}: {new:#x} does not fit an imm8")
        rom[off] = new
    print(f"party  {n} slots, {d} defined; {len(PARTY_COUNT_PATCHES)} count bytes patched")


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


MAP_TABLE = 0x409368                       # g_MapEntries (file offset)
MAP_COUNT, MAP_ROW = 327, 0x38
MAP_TABLE_LITERALS = [0x5ADC, 0x5D5C, 0xEBD8]   # Draw_DebugTestMenu, Debug_LoadMapFromSelection, Map_FindEntry
MAP_ENTRY3_LITERAL = 0x195C                 # &g_MapEntries[3] (new-game map)
MAP_COUNT_CODE = 0xE0AC                     # Map_GetCount: movs r0,#0xFF; adds r0,#0x48 (327)


def apply_maps(rom, path):
    """New maps from tools/mapbuild.py: data records, then the relocated, extended MapEntry table."""
    m = json.load(open(path))
    for rec in m["records"]:
        off = int(rec["offset"], 0)
        data = bytes.fromhex(rec["bytes"])
        if off + len(data) > len(rom):
            sys.exit(f"maps: record at {off:#x} runs past the ROM")
        if any(b != 0xFF for b in rom[off:off + len(data)]):
            sys.exit(f"maps: record at {off:#x} ({rec.get('note', '')}) overlaps used ROM; move MAP_FREE")
        rom[off:off + len(data)] = data
    rows = [bytes.fromhex(r) for r in m["rows"]]
    if not rows:
        return
    stock = bytes(rom[MAP_TABLE:MAP_TABLE + MAP_COUNT * MAP_ROW])
    have = {(stock[i * MAP_ROW], stock[i * MAP_ROW + 1]) for i in range(MAP_COUNT)}
    for r in rows:
        if (r[0], r[1]) in have:
            sys.exit(f"maps: zone {r[0]} area {r[1]} already exists")
        have.add((r[0], r[1]))
    count = MAP_COUNT + len(rows)
    if count > 255 + 255:
        sys.exit(f"maps: {count} entries do not fit Map_GetCount's movs/adds")
    table = int(m["table_offset"], 0)
    new = stock + b"".join(rows)
    if any(b != 0xFF for b in rom[table:table + len(new)]):
        sys.exit(f"maps: table space at {table:#x} is not free")
    rom[table:table + len(new)] = new
    for off in MAP_TABLE_LITERALS:
        cur = struct.unpack_from("<I", rom, off)[0]
        if cur != ROM_BASE + MAP_TABLE:
            sys.exit(f"maps: literal at {off:#x} is {cur:08x}, expected the map table")
        struct.pack_into("<I", rom, off, ROM_BASE + table)
    cur = struct.unpack_from("<I", rom, MAP_ENTRY3_LITERAL)[0]
    if cur != ROM_BASE + MAP_TABLE + 3 * MAP_ROW:
        sys.exit(f"maps: literal at {MAP_ENTRY3_LITERAL:#x} is {cur:08x}, expected &table[3]")
    struct.pack_into("<I", rom, MAP_ENTRY3_LITERAL, ROM_BASE + table + 3 * MAP_ROW)
    if rom[MAP_COUNT_CODE:MAP_COUNT_CODE + 4] != bytes([0xFF, 0x20, MAP_COUNT - 255, 0x30]):
        sys.exit(f"maps: Map_GetCount at {MAP_COUNT_CODE:#x} is {rom[MAP_COUNT_CODE:MAP_COUNT_CODE + 4].hex()}")
    rom[MAP_COUNT_CODE + 2] = count - 255
    for info in m["maps"]:
        print(f"map    zone {info['zone']} area {info['area']} {info['name']} ({info['size'][0]}x{info['size'][1]} px)")
    print(f"maps   table {ROM_BASE + table:08x}: {count} entries, {len(m['records'])} records "
          f"{m['free_start']}..{m['free_end']}")


def main():
    orig, blob, symfile, out = sys.argv[1:5]
    debug_menu = "--debug-menu" in sys.argv[5:]
    wide = "--wide" in sys.argv[5:]
    data_patches = [sys.argv[i + 1] for i, a in enumerate(sys.argv) if a == "--data-patch"]
    maps = [sys.argv[i + 1] for i, a in enumerate(sys.argv) if a == "--maps"]
    party_slots = int(next((sys.argv[i + 1] for i, a in enumerate(sys.argv) if a == "--party-slots"), 6))
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

    for addr, name in ENTRY_JUMPS + PARTY_ENTRY_JUMPS:
        target = syms[name] | 1
        off = addr - ROM_BASE
        if addr in R3_ARG_JUMPS:
            # the 4th argument lives in r3: jump through r12 instead (push {r0}; ldr r0,[pc]; mov r12,r0;
            # pop {r0}; bx r12; .word), 16 bytes at a word-aligned entry, 14 at a halfword-aligned one
            if addr % 4 == 0:
                struct.pack_into("<HHHHHHI", rom, off, 0xB401, 0x4802, 0x4684, 0xBC01, 0x4760, 0x46C0, target)
            else:
                struct.pack_into("<HHHHHI", rom, off, 0xB401, 0x4801, 0x4684, 0xBC01, 0x4760, target)
            print(f"entry  {addr:08x} -> {name} @ {target:08x} (via r12)")
            continue
        if addr % 4 == 0:
            struct.pack_into("<HHI", rom, off, 0x4B00, 0x4718, target)   # ldr r3,[pc,#0]; bx r3; .word
        elif addr % 4 == 2:
            # pc reads as (addr+4)&~3 = addr+2, so [pc,#4] is the word at addr+6
            struct.pack_into("<HHHI", rom, off, 0x4B01, 0x4718, 0x46C0, target)   # ldr r3,[pc,#4]; bx r3; nop; .word
        else:
            sys.exit(f"entry jump at {addr:#x} must be 2-byte aligned")
        print(f"entry  {addr:08x} -> {name} @ {target:08x}")

    build_party_table(rom, party_slots)
    apply_party_patches(rom, syms, party_slots)
    for path in data_patches:
        apply_data_patch(rom, path, syms)
    apply_party_count_patches(rom, party_slots)
    for path in maps:
        apply_maps(rom, path)

    open(out, "wb").write(rom)
    print(f"wrote {out}: {len(rom)} bytes, code {len(code)} bytes at {ROM_BASE + CODE_AT:#x}")


if __name__ == "__main__":
    main()
