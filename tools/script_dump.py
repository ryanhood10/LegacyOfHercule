"""Dialogue and script dumper for Legacy of Goku II (USA): walks a map's dialogue sequences and
bytecode scripts (triggers, town NPCs, enemy defeat actions, the on-load script) and prints readable
text and opcodes. Also emits text-replacement records for tools/patch_rom.py --data-patch.

usage: script_dump.py maps                          list the 327 map entries (index, zone, area, counts)
       script_dump.py map ZONE AREA [--index N]     dump one map (N picks among duplicates of zone/area)
       script_dump.py seq ADDR                      dump one dialogue sequence (ROM address of the pointer array)
       script_dump.py script ADDR                   disassemble one bytecode script
       script_dump.py text ADDR "new text" [--out patch.json]
            replace a NUL-terminated text in place (must fit; padded with NULs); appends the record to --out
       script_dump.py entry SEQ INDEX "new text" --free 0xOFFSET [--out patch.json]
            replace dialogue entry INDEX of sequence SEQ by a new plain-text (mode 1) entry written at
            file offset OFFSET (keeps the entry's next-index and speaker), repointing the sequence slot.
            Texts may use \\xNN escapes (0x0b = page break); keep the leading "@^" the stock lines have.
       --rom PATH selects the ROM (default roms/log2.gba; a built ROM works too)

Formats (file offsets = ROM address - 0x08000000; from the decompile and the Arefu/Legacy notes,
checked on the ROM):
  MapEntry (0x08409368, 327 x 0x38): +0 zone, +1 area, +2 variation count, +3 trigger count,
    +4 script count, +5 item count, +6 object count, +0xC map name id (or script ptr), +0x10 triggers
    (8-byte {kind, data*}), +0x14 mapScripts (ptr array), +0x18 items, +0x1C objects, +0x24 music id
    (or script ptr), +0x2C variation array, +0x30 on-load script.
  Trigger data: +0 condition fn, +4 function fn, +8 x1,y1,x2,y2 (s16), +0x10 payload: a dialogue
    sequence when the condition is 0x0800D7E1, a script when it is 0x0800C2C3.
  mapScripts records: handler 0x0800B711 town NPC (+0xC dialogue sequence, +0x10 sprite id);
    0x0800D6DF script actor (+0xC sprite id); 0x0800E77F enemy (+8 stat, +0xC x,y, +0x10 sprite,
    +0x14 action {func, payload}: 0x0801069B script, 0x080106B3 dialogue sequence, 0x08010699 none).
  Dialogue sequence: array of entry pointers ending in a word whose top 16 bits are 0. Entry:
    u16 next index, u8 mode, then mode 0 = script at +3; 1 = u8 speaker, text at +4; 2 = speaker,
    JCALG1-compressed text at +4; 3 = speaker, u32 format string ptr at +4, script at +8 pushing the
    %s/%d arguments; 4 = like 3 with the format text compressed; 5 = u32 jump target at +4.
  Scripts: main dispatch 0x00 PushByte u8, 0x01 PushVarint (7-bit groups MSB first, even/odd sign fold), 0x02 Step u8 (the 145
    game actions, OpCodes.csv), 0x03 TypeOp u8, 0x04-0x10 arithmetic/compare, 0x11 END, 0x12 Jump s8,
    0x13 JumpIfFalse s8, 0x14 Pop, 0x15-0x1A StepTypePost u8 u8, 0x1B PushToAltStack, 0x1C LoopOrJump s8,
    0x1D PushMultiVarint (count + N varints).
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapchunks import decompress  # noqa: E402  (JCALG1 port, used for mode 2/4 text)

ROM_BASE = 0x08000000
MAP_ENTRIES = 0x08409368
MAP_COUNT = 327
MAP_ENTRY_SIZE = 0x38
TRIGGER_DIALOG_COND = 0x0800D7E1
TRIGGER_SCRIPT_COND = 0x0800C2C3
HANDLER_NPC, HANDLER_ACTOR, HANDLER_ENEMY = 0x0800B711, 0x0800D6DF, 0x0800E77F
ACTION_SCRIPT, ACTION_DIALOG, ACTION_NONE = 0x0801069B, 0x080106B3, 0x08010699

MAIN_OPS = {0x00: "PushByte", 0x01: "PushVarint", 0x02: "Step", 0x03: "TypeOp", 0x04: "And", 0x05: "Not",
            0x06: "Negate", 0x07: "Add", 0x08: "Sub", 0x09: "Mul", 0x0A: "Div", 0x0B: "CmpEq", 0x0C: "CmpNe",
            0x0D: "CmpGt", 0x0E: "CmpGe", 0x0F: "CmpLt", 0x10: "CmpLe", 0x11: "END", 0x12: "Jump",
            0x13: "JumpIfFalse", 0x14: "Pop", 0x15: "StepTypePost_Pop", 0x16: "StepTypePost_IncPeek",
            0x17: "StepTypePost_Peek", 0x18: "StepTypePost_PopDec", 0x19: "StepTypePost_DecPeek",
            0x1A: "StepTypePost_PeekDec", 0x1B: "PushToAltStack", 0x1C: "LoopOrJump", 0x1D: "PushMultiVarint"}

# Step operand -> game action (BytecodeVM_OpcodeTable 0x083B5D28); names follow the Arefu/Legacy IDA database
STEP_NAMES = ["PushAccumulator", "PushGlobalVar0", "PushGlobalVar1", "PushGlobalVar2", "PushScriptArg0", "PushScriptArg1",
    "PushScriptArg2", "PushScriptArg3", "PushCurrentWorld", "PushCurrentArea", "PushCurrentVariant",
    "PushActiveCharIndex", "PushActiveCharName", "PushActiveCharHP", "PushActiveCharMaxHP",
    "PushActiveCharEP", "PushActiveCharMaxEP", "StackRand", "StackRandChance", "StackTestStoryFlag",
    "StackTestStoryFlagNot", "StackGetItemCount", "StackGetCharLevel", "StackTestPartyMemberFlag",
    "DisableLayer", "EnableLayer", "PickUpItem", "SetStoryFlag", "ClearStoryFlag", "FindCharacterEntity",
    "FindEntityByType", "PanCameraToPosition", "CenterCameraOnChar", "SetActiveEntity", "SetMapRestoreFlag",
    "LoadMapWithEntities", "LoadMapNoEntities", "RestoreMap", "WarpToMapWithEntities", "WarpToMapNoEntities",
    "FadeOut", "FadeFromWhite", "FadeToWhite", "WhiteFadeOut", "WhiteFlash", "UploadSpritePalette",
    "PlayAudio", "StopAudio", "StopAllAudio", "PlayMusic", "StopMusic", "SpawnCharacterEntity",
    "DespawnEntity", "WalkToPosition", "FlyToPosition", "SetEntityFacing", "SetEntityFollow",
    "SetEntityAnimation", "EntityWaitFrames", "Deprecated_CharacterSpecialAttack", "EntityWaitTouchPlayer",
    "Deprecated_CharacterFollow", "SetEntityPositionInstant", "BeginCommandBatch", "CommitCommandBatch",
    "PlayAudioBlocking", "Deprecated_ShowImage", "WaitForConfirmButton", "WaitFramesSimple", "WaitFrames",
    "SetActiveCharacter", "SetCollisionRect", "ClearCollisionRect", "SetCharacterTransformation",
    "ReloadMap", "PushActiveCharNameUpper", "RemoveItem", "RemoveItems", "SetBgLayerPriority",
    "SetCharacterInPartyFlag", "SetCharacterFlag_Bit1", "ClearCharacterFlag_Bit1", "SetCharacterFlag_Bit5",
    "SetCharacterFlag_Dynamic", "TriggerMapEvent", "DrawBossHealthBar", "SetPlayerVisible", "SpawnItem",
    "FindPlayerEntity", "WalkByOffset", "DrawBossHealthBarRange", "ClearCharacterInPartyFlag", "SpawnItem",
    "EntityStopMovement", "EntitySetVelocity", "PlayDamageEffect", "NPCFireKiBlastAtEntity",
    "ReturnToTitleScreen", "SetCharacterLevel", "NPCFireKiBlast", "SpawnDustCloud", "sub_800AC5C",
    "sub_800ACAC", "sub_800ACEA", "DespawnCharacterAndRecordPosition", "StackGetCharHP", "StackGetCharEP",
    "StackGetCharMaxHP", "StackGetCharMaxEP", "SetCharHP", "SetCharEP", "PlayAudioVolume", "PlaySFX",
    "EnableMenuAccess", "EnableDragonRadar", "SetActiveCharacterNoReset", "ResetNonActiveSprites",
    "RemoveRectFromMask2", "AddRectToMask2", "GetActiveEntityContext", "sub_800AD3E",
    "SetEntityAnimationNoWait", "DisableWorld", "EnableWorld", "DrawSprite", "PlayCredits",
    "PushActiveCharIsTransformed", "SpawnMapEntity", "SetPartyMemberFlag", "PushSaveStatePtr", "sub_800AFD4",
    "PushLevelUpStat1", "PushLevelUpStat2", "PushLevelUpStat3", "ScreenShake", "FlashScreenColor",
    "GlideToPosition", "NPCFireKamehameha", "EnterSleepMode", "AddEXP", "LockEntityPose",
    "RegisterScouterEntry", "NPCFireBigBangAtEntity", "PushEntityPosition", "SetEntityPosition"]


def is_rom(p):
    return ROM_BASE <= p < ROM_BASE + 0x02000000


def off(p):
    return p - ROM_BASE


def text(b):
    return "".join(chr(c) if 32 <= c < 127 else f"<{c:02x}>" for c in b)


def cstr(rom, o):
    return rom[o:rom.index(b"\0", o)]


def varint(rom, o):
    """The game's varint (BytecodeVM_PushVarint 0x08008F5B): 7-bit groups MSB first while bit 7 is set,
    then the sign fold used by the handler: even v -> v / 2, odd v -> -(v / 2). Returns (value, new offset)."""
    v = 0
    while True:
        b = rom[o]
        o += 1
        v = (v << 7) + (b & 0x7F)
        if not b & 0x80:
            break
    return (-(v >> 1) if v & 1 else v >> 1), o


def disasm(rom, o, indent="    ", limit=512):
    """Print one script; returns the offset after END."""
    start = o
    while o - start < limit:
        op = rom[o]
        name = MAIN_OPS.get(op, f"op{op:02x}?")
        at = f"{indent}{ROM_BASE + o:08x}"
        if op == 0x00:
            print(f"{at}  00 {rom[o + 1]:02x}      {name} {rom[o + 1]}")
            o += 2
        elif op == 0x01:
            v, n = varint(rom, o + 1)
            print(f"{at}  {rom[o:n].hex(' '):11} {name} {v}")
            o = n
        elif op == 0x02:
            k = rom[o + 1]
            sname = STEP_NAMES[k] if k < len(STEP_NAMES) else "?"
            print(f"{at}  02 {k:02x}      {name} {k} {sname}")
            o += 2
        elif op == 0x03:
            print(f"{at}  03 {rom[o + 1]:02x}      {name} {rom[o + 1]}")
            o += 2
        elif op in (0x12, 0x13, 0x1C):
            d = struct.unpack_from("<b", rom, o + 1)[0]
            print(f"{at}  {op:02x} {rom[o + 1]:02x}      {name} {d:+d} -> {ROM_BASE + o + 2 + d:08x}")
            o += 2
        elif 0x15 <= op <= 0x1A:
            k, t = rom[o + 1], rom[o + 2]
            sname = STEP_NAMES[k] if k < len(STEP_NAMES) else "?"
            print(f"{at}  {op:02x} {k:02x} {t:02x}   {name} {k} {sname}, type {t}")
            o += 3
        elif op == 0x1D:
            n = rom[o + 1]
            p = o + 2
            vals = []
            for _ in range(n):
                v, p = varint(rom, p)
                vals.append(v)
            print(f"{at}  {rom[o:p].hex(' '):11} {name} {vals}")
            o = p
        elif op == 0x11:
            print(f"{at}  11         END")
            return o + 1
        elif op in MAIN_OPS:
            print(f"{at}  {op:02x}         {name}")
            o += 1
        else:
            print(f"{at}  {op:02x}         ?? unknown main opcode, stopping")
            return o + 1
    print(f"{indent}... no END within {limit} bytes")
    return o


def jcalg1_text(rom, o):
    try:
        data = decompress(rom, o, 8)
    except Exception as e:  # noqa: BLE001
        return f"<jcalg1 decode failed: {e}>"
    end = data.find(b"\0")
    return text(data if end < 0 else data[:end])


def dump_sequence(rom, seq, indent="  "):
    print(f"{indent}dialogue sequence @ {seq:08x}")
    o = off(seq)
    i = 0
    while True:
        p = struct.unpack_from("<I", rom, o + i * 4)[0]
        if p >> 16 == 0:
            print(f"{indent}  [{i}] end marker {p:#x}")
            break
        if not is_rom(p):
            print(f"{indent}  [{i}] {p:08x} not a ROM pointer, stopping")
            break
        e = off(p)
        nxt, mode = struct.unpack_from("<HB", rom, e)
        head = f"{indent}  [{i}] @{p:08x} -> {nxt:<3} "
        if mode == 0:
            print(head + "SCRIPT")
            disasm(rom, e + 3, indent + "      ")
        elif mode == 1:
            print(head + f"TEXT     speaker {rom[e + 3]:3}: \"{text(cstr(rom, e + 4))}\"  (text @ {p + 4:08x})")
        elif mode == 2:
            print(head + f"TEXT.Z   speaker {rom[e + 3]:3}: \"{jcalg1_text(rom, e + 4)}\"")
        elif mode in (3, 4):
            fmt = struct.unpack_from("<I", rom, e + 4)[0]
            if mode == 3:
                s = text(cstr(rom, off(fmt))) if is_rom(fmt) else f"<bad fmt ptr {fmt:08x}>"
            else:
                s = jcalg1_text(rom, off(fmt)) if is_rom(fmt) else f"<bad fmt ptr {fmt:08x}>"
            print(head + f"TEXT.FMT{'.Z' if mode == 4 else '  '} speaker {rom[e + 3]:3}: \"{s}\"  (fmt @ {fmt:08x})")
            disasm(rom, e + 8, indent + "      ")
        elif mode == 5:
            tgt = struct.unpack_from("<I", rom, e + 4)[0]
            print(head + f"JUMP     target {tgt:#x}")
        else:
            print(head + f"mode {mode}? bytes {rom[e:e + 12].hex()}")
        i += 1
        if i > 400:
            print(f"{indent}  ... stopping after 400 entries")
            break


def script_or_sequence(rom, p, indent):
    """Trigger payloads of unknown kind: a sequence starts with ROM pointers, a script with opcodes."""
    w = struct.unpack_from("<I", rom, off(p))[0]
    if is_rom(w):
        dump_sequence(rom, p, indent)
    else:
        print(f"{indent}script @ {p:08x}")
        disasm(rom, off(p), indent + "  ")


def map_entry(rom, idx):
    o = off(MAP_ENTRIES) + idx * MAP_ENTRY_SIZE
    e = rom[o:o + MAP_ENTRY_SIZE]
    w = lambda k: struct.unpack_from("<I", e, k)[0]
    return dict(index=idx, addr=MAP_ENTRIES + idx * MAP_ENTRY_SIZE, zone=e[0], area=e[1], variations=e[2],
                triggers=e[3], scripts=e[4], items=e[5], objects=e[6], name=w(0xC), trig_ptr=w(0x10),
                script_ptr=w(0x14), item_ptr=w(0x18), object_ptr=w(0x1C), music=w(0x24), var_ptr=w(0x2C),
                onload=w(0x30))


def list_maps(rom):
    print("idx  zone area var trig scr item obj  name      music     onload")
    for i in range(MAP_COUNT):
        m = map_entry(rom, i)
        print(f"{i:3}  {m['zone']:4} {m['area']:4} {m['variations']:3} {m['triggers']:4} {m['scripts']:3} "
              f"{m['items']:4} {m['objects']:3}  {m['name']:<9x} {m['music']:<9x} "
              f"{m['onload']:08x}" if m['onload'] else
              f"{i:3}  {m['zone']:4} {m['area']:4} {m['variations']:3} {m['triggers']:4} {m['scripts']:3} "
              f"{m['items']:4} {m['objects']:3}  {m['name']:<9x} {m['music']:<9x} -")


def dump_map(rom, zone, area, which=0):
    matches = [i for i in range(MAP_COUNT) if rom[off(MAP_ENTRIES) + i * MAP_ENTRY_SIZE] == zone
               and rom[off(MAP_ENTRIES) + i * MAP_ENTRY_SIZE + 1] == area]
    if not matches:
        sys.exit(f"no map entry for zone {zone} area {area}")
    m = map_entry(rom, matches[min(which, len(matches) - 1)])
    print(f"map entry {m['index']} @ {m['addr']:08x}: zone {m['zone']} area {m['area']}, {m['variations']} variation(s), "
          f"{m['triggers']} triggers, {m['scripts']} scripts, {m['items']} items, {m['objects']} objects"
          + (f"  ({len(matches)} entries share this zone/area: {matches})" if len(matches) > 1 else ""))
    for label, v in (("map name", m["name"]), ("music", m["music"])):
        if is_rom(v):
            print(f"{label}: script @ {v:08x}")
            disasm(rom, off(v))
        else:
            print(f"{label}: id {v:#x}")
    if m["onload"]:
        print(f"on-load script @ {m['onload']:08x}")
        disasm(rom, off(m["onload"]))

    for t in range(m["triggers"]):
        kind, data = struct.unpack_from("<II", rom, off(m["trig_ptr"]) + t * 8)
        d = off(data)
        cond, func, x1, y1, x2, y2, payload = struct.unpack_from("<IIhhhhI", rom, d)
        print(f"\ntrigger {t} @ {data:08x}: kind {kind:#x} condition {cond:08x} function {func:08x} "
              f"rect ({x1},{y1})-({x2},{y2}) payload {payload:08x}")
        if not is_rom(payload):
            continue
        if cond == TRIGGER_DIALOG_COND:
            dump_sequence(rom, payload)
        elif cond == TRIGGER_SCRIPT_COND:
            print(f"  script @ {payload:08x}")
            disasm(rom, off(payload))
        else:
            script_or_sequence(rom, payload, "  ")

    for s in range(m["scripts"]):
        rec = struct.unpack_from("<I", rom, off(m["script_ptr"]) + s * 4)[0]
        r = off(rec)
        handler, flags = struct.unpack_from("<II", rom, r)
        if handler == HANDLER_NPC:
            x, y, dlg, sprite = struct.unpack_from("<hhII", rom, r + 8)
            print(f"\nscript {s} @ {rec:08x}: NPC sprite {sprite} at ({x},{y}) flags {flags:#x} dialogue {dlg:08x}")
            if is_rom(dlg):
                dump_sequence(rom, dlg)
        elif handler == HANDLER_ACTOR:
            x, y, sprite = struct.unpack_from("<hhI", rom, r + 8)
            print(f"\nscript {s} @ {rec:08x}: actor sprite {sprite} at ({x},{y}) flags {flags:#x}")
        elif handler == HANDLER_ENEMY:
            stat, x, y, sprite, action = struct.unpack_from("<IhhII", rom, r + 8)
            print(f"\nscript {s} @ {rec:08x}: enemy stat {stat} sprite {sprite} at ({x},{y}) flags {flags:#x}", end="")
            if is_rom(action):
                func, payload = struct.unpack_from("<II", rom, off(action))
                if func == ACTION_DIALOG:
                    print(f" on defeat: dialogue")
                    dump_sequence(rom, payload)
                elif func == ACTION_SCRIPT:
                    print(f" on defeat: script @ {payload:08x}")
                    disasm(rom, off(payload))
                else:
                    print(f" action {func:08x} payload {payload:08x}")
            else:
                print()
        else:
            print(f"\nscript {s} @ {rec:08x}: handler {handler:08x} (not decoded)")


# ---- write path ----------------------------------------------------------------------------------

def add_records(out_path, recs):
    existing = json.load(open(out_path)) if out_path and os.path.exists(out_path) else []
    existing += recs
    if out_path:
        json.dump(existing, open(out_path, "w"), indent=1)
        print(f"{out_path}: {len(existing)} records")
    else:
        print(json.dumps(recs, indent=1))


def unescape(t):
    """Allow \\xNN escapes for the text-box control bytes (0x0b = page break in the stock text)."""
    return t.encode("latin-1").decode("unicode_escape")


def replace_text(rom, addr, new, out_path):
    new = unescape(new)
    o = off(addr)
    old = cstr(rom, o)
    data = new.encode("latin-1")
    if len(data) > len(old):
        sys.exit(f"new text is {len(data)} bytes, only {len(old)} fit at {addr:08x} (old: {text(old)!r})")
    rec = {"offset": f"{o:#x}", "bytes": (data + b"\0" * (len(old) + 1 - len(data))).hex(),
           "note": f"text @ {addr:08x}: {new[:50]!r}"}
    print(f"old: {text(old)!r}\nnew: {new!r}")
    add_records(out_path, [rec])


def replace_entry(rom, seq, index, new, free, out_path):
    new = unescape(new)
    slot = off(seq) + index * 4
    old_entry = struct.unpack_from("<I", rom, slot)[0]
    if not is_rom(old_entry):
        sys.exit(f"sequence {seq:08x} slot {index} holds {old_entry:#x}, not an entry")
    e = off(old_entry)
    nxt, mode = struct.unpack_from("<HB", rom, e)
    speaker = rom[e + 3] if mode in (1, 2, 3, 4) else 0
    data = new.encode("latin-1")
    entry = struct.pack("<HBB", nxt, 1, speaker) + data + b"\0"
    entry += b"\0" * (-len(entry) % 4)
    free = (free + 3) & ~3
    recs = [{"offset": f"{free:#x}", "bytes": entry.hex(),
             "note": f"dialogue entry (next {nxt}, speaker {speaker}) {new[:50]!r}"},
            {"offset": f"{slot:#x}", "expect": struct.pack("<I", old_entry).hex(), "word": f"{ROM_BASE + free:#010x}",
             "note": f"sequence {seq:08x}[{index}] -> new entry"}]
    print(f"old entry @ {old_entry:08x} mode {mode} next {nxt} speaker {speaker}; new entry at file {free:#x} "
          f"({len(entry)} bytes), next free {free + len(entry):#x}")
    add_records(out_path, recs)


def main():
    args = sys.argv[1:]
    rom_path = "roms/log2.gba"
    if "--rom" in args:
        i = args.index("--rom")
        rom_path = args[i + 1]
        del args[i:i + 2]
    out_path = None
    if "--out" in args:
        i = args.index("--out")
        out_path = args[i + 1]
        del args[i:i + 2]
    if not args:
        sys.exit(__doc__)
    rom = open(rom_path, "rb").read()
    cmd = args[0]
    if cmd == "maps":
        list_maps(rom)
    elif cmd == "map":
        which = 0
        if "--index" in args:
            i = args.index("--index")
            which = int(args[i + 1])
            del args[i:i + 2]
        dump_map(rom, int(args[1]), int(args[2]), which)
    elif cmd == "seq":
        dump_sequence(rom, int(args[1], 0), "")
    elif cmd == "script":
        disasm(rom, off(int(args[1], 0)), "")
    elif cmd == "text":
        replace_text(rom, int(args[1], 0), args[2], out_path)
    elif cmd == "entry":
        i = args.index("--free")
        free = int(args[i + 1], 0)
        del args[i:i + 2]
        replace_entry(rom, int(args[1], 0), int(args[2], 0), args[3], free, out_path)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
