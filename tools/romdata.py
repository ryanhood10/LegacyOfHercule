"""ROM data editor for the character tables of Legacy of Goku II (USA): the new-game party, the
per-slot level-up growth rows, the ability table and the level EXP table. Reads and prints them from
a ROM, and turns a design spec (JSON) into a data patch file that tools/patch_rom.py applies with
--data-patch (records of file offset + bytes, see that file's header).

usage: romdata.py party     [ROM]              default party (CharacterEntry per slot) + growth rows
       romdata.py abilities [ROM]              ability table (icons, functions, EP cost, animation)
       romdata.py exp       [ROM]              level EXP table (50 levels)
       romdata.py build SPEC.json OUT.json [ROM] [--party-slots N]   spec -> data patch records
       ROM defaults to roms/log2.gba; a built ROM works too (the ability and party tables follow
       their relocation).

Party slots: a build with PARTY_SLOTS=N (tools/patch_rom.py --party-slots N) moves the default party
and growth rows to an N-entry table at file PARTY_TABLE_AT (stock rows 0-5, zero entries above) and
repoints the literal at 0x3EA0 so literal + 0xA8 is still the growth rows. `build --party-slots N`
writes its party/growth records into that table (slots 0..N-1); without the flag they edit the stock
six-entry table in place.

Spec file (all sections optional; numbers may be JSON ints or "0x.." strings):
{
  "free": "0x900000",                 file offset where relocated/new data is written (16 MiB output ROM)
  "party":  {"5": {"hp": 50, "ep": 30, "str": 2.0, "pow": 0.5, "end": 1.5, "display_row": 21,
                   "flags": "0x40", "abilities": [14, 15, 16, 17], "ability_index": 0}},
  "growth": {"5": {"str": [440, 640], "pow": [300, 500], "end": [500, 700]}},   per-level min/max, /256
  "exp":    {"10": 4350},             EXP needed to reach level N (N = 2..50)
  "abilities": [{"name": "Taunt", "icons": 11, "is_ready": "fn:hercule_taunt_ready",
                 "on_use": "fn:hercule_taunt_use"}],
            new entries appended after the 14 stock ones (indices 14, 15, ...); "icons" copies the icon
            pair of stock ability N or is ["0x...", "0x..."] (or "sym:<symbol>" each); functions are "fn:<symbol>" (coop.sym,
            Thumb bit added by patch_rom.py) or addresses. The table is relocated to "free" and the
            two code literals that point at it are repointed.
  "strings": [{"offset": "0x3B81B0", "text": "..."}],   in-place text replacement, must fit the old string
  "patches": [{"offset": "0x3EC738", "expect": "5b", "bytes": "4f", "note": "..."}],  raw records, passed through
  "include": ["data/hercule-text.json"]   more record files (e.g. written by tools/script_dump.py), appended;
            a file holding an object instead of a list is a story spec, applied by tools/mapedit.py
            (map table edits, lscript sources assembled into "story_free", default 0xB00000)
}

Facts (file offsets; from the decompile and the Arefu/Legacy notes, verified on the ROM):
  0x1D9548 CharacterDefaultStats_Table, 6 x 28-byte CharacterEntry; the only code reference is the
           literal at 0x3EA0, which the level-up code also uses to reach the growth rows:
  0x1D95F0 growth rows, 6 x {u16 min, u16 max} x 3 (str, pow, end): per level-up a stat gains
           rand(max - min + 1) + min (one draw for all the levels gained), capped at 25600 (100.0).
           HP grows by max_hp * (0x0E14 + rand(0x51F)) / 65536 per level (5.5 % .. 7.5 %, shared by
           every slot); EP by rand(3) + 2.
  0x1D9834 level EXP table, 50 words (EXP needed for level i + 1), literal at 0x42EC.
  0x1D9900 g_AbilityTable, 14 x {readyIcon, coolingIcon, isReady, onUse}; literals at 0x8024 and 0x8E8C.
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))   # tools/mapedit.py for story specs

ROM_BASE = 0x08000000
PARTY_OFF = 0x1D9548
PARTY_REF = 0x3EA0
GROWTH_OFF = 0x1D95F0
EXP_OFF = 0x1D9834
EXP_REF = 0x42EC
ABILITY_OFF = 0x1D9900
ABILITY_REFS = (0x8024, 0x8E8C)
STOCK_ABILITIES = 14
STOCK_ABILITY_NAMES = ["BigBang", "BurningAttack", "KiBlast", "Kamehameha", "Masenko", "(none)", "HerculePunch",
                       "EnergyPunch", "ScatterShot", "SpecialBeamCannon", "SpiritBomb", "Transformation",
                       "TransformationB", "SwordBlast"]
ENTRY = struct.Struct("<HHHHBBHHHII4B")   # CharacterEntry
PARTY_TABLE_AT = 0xA00000   # file offset of the N-entry default party table in a built ROM (then growth rows)
PARTY_STOCK_SLOTS = 6
GROWTH_AT_LITERAL = 0xA8    # the level-up code reads growth rows at literal(0x3EA0) + 0xA8


def party_table_layout(n):
    """(entries offset, growth offset, literal value) of the relocated N-entry table."""
    return PARTY_TABLE_AT, PARTY_TABLE_AT + n * ENTRY.size, ROM_BASE + PARTY_TABLE_AT + n * ENTRY.size - GROWTH_AT_LITERAL


def party_slots_of(rom):
    """Slot count of a ROM: 6 for the stock table, else derived from the relocated literal."""
    lit = rom_ptr(rom, PARTY_REF)
    if lit == ROM_BASE + PARTY_OFF:
        return PARTY_STOCK_SLOTS
    return (lit + GROWTH_AT_LITERAL - ROM_BASE - PARTY_TABLE_AT) // ENTRY.size


def party_offsets(rom):
    """(entries offset, growth offset, slot count) for reading a stock or built ROM."""
    n = party_slots_of(rom)
    if n == PARTY_STOCK_SLOTS and rom_ptr(rom, PARTY_REF) == ROM_BASE + PARTY_OFF:
        return PARTY_OFF, GROWTH_OFF, n
    ent, grow, _ = party_table_layout(n)
    return ent, grow, n


def num(v):
    return int(v, 0) if isinstance(v, str) else v


def rom_ptr(rom, off):
    return struct.unpack_from("<I", rom, off)[0]


def read_entry(rom, slot, table=None):
    if table is None:
        table = party_offsets(rom)[0]
    if slot >= PARTY_STOCK_SLOTS and table == PARTY_OFF:
        return dict(hp=0, max_hp=0, ep=0, max_ep=0, display_row=0, ability_index=0, str=0, pow=0, end=0, exp=0,
                    flags=0, abilities=[0, 0, 0, 0])
    f = ENTRY.unpack_from(rom, table + slot * 28)
    return dict(hp=f[0], max_hp=f[1], ep=f[2], max_ep=f[3], display_row=f[4], ability_index=f[5], str=f[6], pow=f[7],
                end=f[8], exp=f[9], flags=f[10], abilities=list(f[11:15]))


def pack_entry(e):
    return ENTRY.pack(e["hp"], e["max_hp"], e["ep"], e["max_ep"], e["display_row"], e["ability_index"], e["str"],
                      e["pow"], e["end"], e["exp"], e["flags"], *e["abilities"])


def read_growth(rom, slot, table=None):
    if table is None:
        table = party_offsets(rom)[1]
    if slot >= PARTY_STOCK_SLOTS and table == GROWTH_OFF:
        return {"str": [0, 0], "pow": [0, 0], "end": [0, 0]}
    v = struct.unpack_from("<6H", rom, table + slot * 12)
    return {"str": [v[0], v[1]], "pow": [v[2], v[3]], "end": [v[4], v[5]]}


def pack_growth(g):
    return struct.pack("<6H", *g["str"], *g["pow"], *g["end"])


def ability_table_offset(rom):
    """Follow the code literal, so a built ROM with a relocated table prints the relocated one."""
    return rom_ptr(rom, ABILITY_REFS[0]) - ROM_BASE


def read_abilities(rom):
    off = ability_table_offset(rom)
    out = []
    while off + 16 <= len(rom):
        ready, cooling, is_ready, on_use = struct.unpack_from("<IIII", rom, off)
        if not (ROM_BASE <= ready < ROM_BASE + 0x02000000 and ROM_BASE <= is_ready < ROM_BASE + 0x02000000):
            break
        out.append((ready, cooling, is_ready, on_use))
        off += 16
    return out


def thumb_cost_and_anim(rom, fn):
    """Stock onUse stubs are `ep -= N; SetAnimation(entity, ANIM)`: recover N and ANIM from the Thumb
    halfwords (subs rX,#N then movs r1,#ANIM before the first bl). Returns (cost, anim) or Nones."""
    if not (ROM_BASE <= fn < ROM_BASE + len(rom)):
        return None, None
    off = (fn & ~1) - ROM_BASE
    cost = anim = None
    for i in range(24):
        hw = struct.unpack_from("<H", rom, off + i * 2)[0]
        if hw & 0xF800 == 0x3800 and cost is None:      # subs rX, #imm
            cost = hw & 0xFF
        elif hw & 0xFF00 == 0x2100:                      # movs r1, #imm
            anim = hw & 0xFF
        elif hw & 0xF800 == 0xF000:                      # bl (first half)
            break
    return cost, anim


def thumb_ready_cost(rom, fn):
    if not (ROM_BASE <= fn < ROM_BASE + len(rom)):
        return None
    off = (fn & ~1) - ROM_BASE
    for i in range(16):
        hw = struct.unpack_from("<H", rom, off + i * 2)[0]
        if hw & 0xFF00 == 0x2900:                        # cmp r1, #imm
            return hw & 0xFF
        if hw == 0x4770:                                 # bx lr
            break
    return None


def read_exp(rom):
    return list(struct.unpack_from("<50I", rom, EXP_OFF))


# ---- printing ------------------------------------------------------------------------------------

def show_party(rom):
    ent, grow, n = party_offsets(rom)
    print(f"default party @ file {ent:#x} (ROM {ROM_BASE + ent:#x}), {n} slots, growth rows @ {grow:#x}, literal at "
          f"{PARTY_REF:#x} = {rom_ptr(rom, PARTY_REF):#010x}")
    print("slot row  hp/max   ep/max   str    pow    end    exp  flags  sel abilities   growth str/pow/end (min-max /256)")
    for s in range(n):
        e = read_entry(rom, s, ent)
        g = read_growth(rom, s, grow)
        gs = " ".join(f"{g[k][0]}-{g[k][1]}" for k in ("str", "pow", "end"))
        print(f"{s:4} {e['display_row']:3} {e['hp']:4}/{e['max_hp']:<4} {e['ep']:4}/{e['max_ep']:<4} "
              f"{e['str'] / 256:5.2f} {e['pow'] / 256:5.2f} {e['end'] / 256:5.2f} {e['exp']:6} {e['flags']:#06x} "
              f"{e['ability_index']:3} {e['abilities']}  {gs}")


def show_abilities(rom):
    off = ability_table_offset(rom)
    print(f"ability table @ file {off:#x} (ROM {ROM_BASE + off:#x}); literals {ABILITY_REFS[0]:#x}/{ABILITY_REFS[1]:#x} = "
          f"{rom_ptr(rom, ABILITY_REFS[0]):#010x} {rom_ptr(rom, ABILITY_REFS[1]):#010x}")
    print(" idx name              readyIcon  coolIcon   isReady    onUse      cost(ready/use) anim")
    for i, (ready, cooling, is_ready, on_use) in enumerate(read_abilities(rom)):
        name = STOCK_ABILITY_NAMES[i] if i < len(STOCK_ABILITY_NAMES) else "(new)"
        rc = thumb_ready_cost(rom, is_ready)
        uc, anim = thumb_cost_and_anim(rom, on_use)
        fmt = lambda v: "-" if v is None else str(v)
        print(f"{i:4} {name:17} {ready:#010x} {cooling:#010x} {is_ready:#010x} {on_use:#010x} "
              f"{fmt(rc):>5}/{fmt(uc):<9} {fmt(anim)}")


def show_exp(rom):
    print(f"level EXP table @ file {EXP_OFF:#x}, literal at {EXP_REF:#x} = {rom_ptr(rom, EXP_REF):#010x}")
    t = read_exp(rom)
    for i in range(0, 50, 5):
        print("  " + "  ".join(f"L{i + k + 1:2}:{t[i + k]:>9}" for k in range(5)))


# ---- spec -> patch -------------------------------------------------------------------------------

def build(rom, spec, party_slots=None):
    recs = []
    free = num(spec.get("free", 0x900000))
    free = (free + 3) & ~3
    if party_slots:
        ent_off, grow_off, _ = party_table_layout(party_slots)
        src_ent, src_grow = PARTY_OFF, GROWTH_OFF     # stock rows: the relocated table starts as their copy
    else:
        ent_off, grow_off = PARTY_OFF, GROWTH_OFF
        src_ent, src_grow = PARTY_OFF, GROWTH_OFF
        party_slots = PARTY_STOCK_SLOTS

    def hexw(v):
        return f"{v:#010x}"

    for slot_s, edits in spec.get("party", {}).items():
        slot = int(slot_s)
        if slot >= party_slots >= 8:   # a smaller test build (e.g. PARTY_SLOTS=8 without Super Saiyan 2 Gohan)
            print(f"party slot {slot}: skipped, the build has {party_slots} slots (PARTY_SLOTS)", file=sys.stderr)
            continue
        if not 0 <= slot < party_slots:
            sys.exit(f"party slot {slot}: the build has {party_slots} slots (PARTY_SLOTS)")
        e = read_entry(rom, slot, src_ent)
        for k, v in edits.items():
            if k == "hp":
                e["hp"] = e["max_hp"] = num(v)
            elif k == "ep":
                e["ep"] = e["max_ep"] = num(v)
            elif k in ("str", "pow", "end"):
                e[k] = int(round(v * 256)) if isinstance(v, float) else num(v)
            elif k == "abilities":
                e["abilities"] = [num(x) for x in v] + [2] * (4 - len(v)) if len(v) < 4 else [num(x) for x in v]
            elif k in ("display_row", "ability_index", "flags", "exp"):
                e[k] = num(v)
            elif k == "_":
                pass   # comment
            else:
                sys.exit(f"party slot {slot}: unknown field {k!r}")
        recs.append({"offset": f"{ent_off + slot * 28:#x}", "bytes": pack_entry(e).hex(),
                     "note": f"default party slot {slot}: hp {e['hp']} ep {e['ep']} str/pow/end "
                             f"{e['str'] / 256:.2f}/{e['pow'] / 256:.2f}/{e['end'] / 256:.2f} abilities {e['abilities']}"})

    for slot_s, g in spec.get("growth", {}).items():
        slot = int(slot_s)
        if slot >= party_slots >= 8:
            print(f"growth slot {slot}: skipped, the build has {party_slots} slots (PARTY_SLOTS)", file=sys.stderr)
            continue
        if not 0 <= slot < party_slots:
            sys.exit(f"growth slot {slot}: the build has {party_slots} slots (PARTY_SLOTS)")
        cur = read_growth(rom, slot, src_grow)
        for k in ("str", "pow", "end"):
            if k in g:
                cur[k] = [num(g[k][0]), num(g[k][1])]
        recs.append({"offset": f"{grow_off + slot * 12:#x}", "bytes": pack_growth(cur).hex(),
                     "note": f"growth slot {slot}: str {cur['str']} pow {cur['pow']} end {cur['end']} per level (/256)"})

    if spec.get("exp"):
        t = read_exp(rom)
        for lvl_s, v in spec["exp"].items():
            lvl = int(lvl_s)
            if not 2 <= lvl <= 50:
                sys.exit(f"exp: level {lvl} out of range 2..50")
            t[lvl - 1] = num(v)
        recs.append({"offset": f"{EXP_OFF:#x}", "bytes": struct.pack("<50I", *t).hex(), "note": "level EXP table"})

    new_abilities = spec.get("abilities", [])
    if new_abilities:
        stock = read_abilities(rom)[:STOCK_ABILITIES]
        table = bytearray()
        for ready, cooling, is_ready, on_use in stock:
            table += struct.pack("<IIII", ready, cooling, is_ready, on_use)
        table_off = free
        fn_fixups = []
        for i, a in enumerate(new_abilities):
            idx = STOCK_ABILITIES + i
            icons = a["icons"]
            entry_off = table_off + idx * 16
            if isinstance(icons, list):
                pair = []
                for k, ic in enumerate(icons):
                    if isinstance(ic, str) and ic.startswith("sym:"):
                        fn_fixups.append({"offset": f"{entry_off + 4 * k:#x}", "word": ic,
                                          "note": f"ability {idx} {a.get('name', '')} icon {k}"})
                        pair.append(0)
                    else:
                        pair.append(num(ic))
                ready, cooling = pair
            else:
                ready, cooling = stock[num(icons)][0], stock[num(icons)][1]
            words = [ready, cooling]
            for k, fn in (("is_ready", a["is_ready"]), ("on_use", a["on_use"])):
                if isinstance(fn, str) and (fn.startswith("fn:") or fn.startswith("sym:")):
                    fn_fixups.append({"offset": f"{entry_off + 8 + (4 if k == 'on_use' else 0):#x}", "word": fn,
                                      "note": f"ability {idx} {a.get('name', '')} {k}"})
                    words.append(0)
                else:
                    words.append(num(fn))
            table += struct.pack("<IIII", *words)
        recs.append({"offset": f"{table_off:#x}", "bytes": table.hex(),
                     "note": f"ability table relocated: {len(stock)} stock + {len(new_abilities)} new entries"})
        recs += fn_fixups
        for ref in ABILITY_REFS:
            recs.append({"offset": f"{ref:#x}", "expect": struct.pack("<I", ROM_BASE + ABILITY_OFF).hex(),
                         "word": hexw(ROM_BASE + table_off), "note": "ability table literal"})
        free = (table_off + len(table) + 3) & ~3

    for s in spec.get("strings", []):
        off = num(s["offset"])
        end = rom.index(b"\0", off)
        room = end - off
        text = s["text"].encode("latin-1")
        if len(text) > room:
            sys.exit(f"string at {off:#x}: {len(text)} bytes do not fit the {room} available")
        recs.append({"offset": f"{off:#x}", "bytes": (text + b"\0" * (room + 1 - len(text))).hex(),
                     "note": s.get("note", f"text {s['text'][:40]!r}")})

    for p in spec.get("patches", []):
        recs.append(dict(p))
    editor = None
    for path in spec.get("include", []):
        data = json.load(open(path))
        if isinstance(data, dict):        # a story spec (data/story/*.json): map edits by tools/mapedit.py
            import mapedit
            got, editor = mapedit.build(rom, [path], num(spec.get("story_free", mapedit.STORY_FREE)),
                                        num(spec.get("story_limit", mapedit.STORY_LIMIT)), editor)
            recs += got
        else:
            recs += data
    return recs


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    cmd = args[0]
    if cmd == "build":
        party_slots = None
        if "--party-slots" in args:
            i = args.index("--party-slots")
            party_slots = int(args[i + 1])
            del args[i:i + 2]
        spec_path, out_path = args[1], args[2]
        rom_path = args[3] if len(args) > 3 else "roms/log2.gba"
        rom = open(rom_path, "rb").read()
        recs = build(rom, json.load(open(spec_path)), party_slots)
        json.dump(recs, open(out_path, "w"), indent=1)
        print(f"{out_path}: {len(recs)} records from {spec_path}")
        for r in recs:
            print(f"  {r['offset']:>10} {r.get('note', '')}")
        return
    rom_path = args[1] if len(args) > 1 else "roms/log2.gba"
    rom = open(rom_path, "rb").read()
    if cmd == "party":
        show_party(rom)
    elif cmd == "abilities":
        show_abilities(rom)
    elif cmd == "exp":
        show_exp(rom)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
