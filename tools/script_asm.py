"""Script assembler for Legacy of Goku II (USA): a readable text form ("lscript") for the game's
bytecode scripts and dialogue sequences, the disassembler that writes it, the assembler that turns it
back into bytes (placed in free ROM as tools/patch_rom.py --data-patch records), and a round-trip test
over every stock script and sequence in the ROM. tools/mapedit.py uses this module to edit maps.

usage: script_asm.py dump map ZONE AREA [--index N]    a map's scripts and sequences as lscript text
       script_asm.py dump script ADDR [NAME]            one bytecode script
       script_asm.py dump seq ADDR [NAME]               one dialogue sequence
       script_asm.py asm FILE.lscript --free 0xOFFSET [--out patch.json]
            assemble every script/sequence in FILE into free ROM (prints the address of each name)
       script_asm.py roundtrip [-v]                    dump -> assemble -> compare, for every stock
            script and dialogue sequence reachable from the map table, items, objects and quests
       --rom PATH selects the ROM (default roms/log2.gba)

The text form (docs/scripts.md has the full reference):

    ; comment                              (';' to the end of the line, outside strings)
    script NAME [@08xxxxxx]                a bytecode script; @ADDR = where the stock copy lives
        PushByte 60                        raw instructions: PushByte, PushVarint, PushMulti a, b, ...,
        Step WaitFrames                      Step NAME|N, TypeOp N, And .. CmpLe, Pop, PushToAltStack,
        WalkToPosition 0, 96, 176          Jump/JumpIfFalse/LoopOrJump LABEL, StepTypePost_* NAME, T,
      loop:                                  END, .bytes HEX; and the macro form "STEPNAME args":
        JumpIfFalse loop                     the arguments pushed (one 0..255 -> PushByte, one other ->
    end                                      PushVarint, several -> PushMulti), then the Step
                                           (END is appended when the last instruction is not END)
    sequence NAME [@08xxxxxx] [end=N]      a dialogue sequence; end = the terminator word (default 0)
      [0] script                           entry lines: [index] (optional, checked) KIND ARGS
          SetPlayerVisible 0                 [-> NEXT] (default index + 1) [@ADDR] then the text;
      [1] text 63 "GOKU: Oh, hi!"            the instructions of script/fmt/fmtz entries follow
      [2] textz 1 @083b9410 "Dad!!\\x0b..."  text SPEAKER "..."       mode 1 plain text
      [3] fmt 113 fmt=@083b5fc0 "Hi, %s!"    textz SPEAKER @A "..."   mode 2 (stock compressed text)
          PushActiveCharName                 fmt SPEAKER [fmt=@A] "..."   mode 3 (%s/%d args script)
      [4] jump 0x12                          fmtz SPEAKER fmt=@A "..."    mode 4 (compressed format)
    end                                      jump TARGET [pad=N]          mode 5

Text strings take \\xNN escapes (\\x0b = page break), \\" and \\\\; keep the "@^" / "#" / "!" / "@" box
prefixes the stock lines use. The assembler never compresses: a textz/fmtz entry whose text still
equals the stock entry at its @ADDR keeps the stock compressed bytes, an edited one becomes a plain
mode 1 / mode 3 entry. Placement (asm, mapedit): an item whose assembled bytes equal the stock bytes at
its @ADDR is not written again; its stock address is used.

Facts used (docs/scripts.md; decompiled from the ROM): Jump / JumpIfFalse add an unsigned byte to
the address after the operand (forward only); LoopOrJump subtracts it (backward, decrement-and-loop
counter at VM +0x44); PushVarint / PushMulti values are 7-bit groups MSB first with the sign fold
(even u -> u/2, odd u -> -(u/2)). Dialogue entries: u16 next, u8 mode, then mode 0 script at +3;
1 speaker + text at +4; 2 speaker + JCALG1 stream at +4 (word-aligned); 3 speaker, u32 format string,
script at +8; 4 like 3 with a JCALG1 format text; 5 pad, u32 target.
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mapchunks import decompress  # noqa: E402
from script_dump import (ROM_BASE, MAP_ENTRIES, MAP_COUNT, MAP_ENTRY_SIZE, MAIN_OPS, STEP_NAMES,  # noqa: E402
                         TRIGGER_DIALOG_COND, TRIGGER_SCRIPT_COND, HANDLER_NPC, HANDLER_ACTOR, HANDLER_ENEMY,
                         ACTION_SCRIPT, ACTION_DIALOG, ACTION_NONE, is_rom, off, map_entry)

TRIGGER_SAVE_COND = 0x08010AC7       # save pad: a 16-byte trigger with no payload
ITEM_FNS = (0x0800F9A3, 0x0800F9C7)  # item records: +0 condition, +8 create fn, +0xC script or sequence
OBJECT_FN = 0x0800FFE7               # object records: +0 condition, +0x10 create fn, +0x14 dialogue
QUEST_TABLE, QUEST_COUNT = 0x086D68DC, 43

# unique display names for the Step table (the table has "SpawnItem" twice: 87 and 92)
STEP_DISPLAY = []
for _i, _n in enumerate(STEP_NAMES):
    STEP_DISPLAY.append(_n if _n not in STEP_DISPLAY else f"{_n}_{_i}")
STEP_BY_NAME = {n: i for i, n in enumerate(STEP_DISPLAY)}
OP_BY_NAME = {n: o for o, n in MAIN_OPS.items()}
OP_BY_NAME["PushMulti"] = 0x1D
SIMPLE_OPS = {o for o in range(0x04, 0x11)} | {0x14, 0x1B}
JUMP_OPS = (0x12, 0x13, 0x1C)
STP_OPS = range(0x15, 0x1B)
ENTRY_KINDS = ("script", "text", "textz", "fmt", "fmtz", "jump")
MODE_OF = {"script": 0, "text": 1, "textz": 2, "fmt": 3, "fmtz": 4, "jump": 5}


class AsmError(Exception):
    pass


# ---- varints -------------------------------------------------------------------------------------

def varint_decode(rom, o):
    v = 0
    start = o
    while True:
        b = rom[o]
        o += 1
        v = (v << 7) | (b & 0x7F)
        if not b & 0x80:
            break
    return (-(v >> 1) if v & 1 else v >> 1), bytes(rom[start:o])


def varint_encode(v):
    u = v * 2 if v >= 0 else -v * 2 + 1
    groups = [u & 0x7F]
    u >>= 7
    while u:
        groups.append(u & 0x7F)
        u >>= 7
    groups.reverse()
    return bytes([g | 0x80 for g in groups[:-1]] + [groups[-1]])


# ---- instructions --------------------------------------------------------------------------------
# An instruction is a tuple: ("byte", v) ("varint", v) ("multi", [v..]) ("step", k) ("typeop", t)
# ("op", opcode) ("jump", opcode, label) ("stp", opcode, k, t) ("raw", bytes) ("end",)

def ins_size(ins):
    k = ins[0]
    if k == "byte" or k == "step" or k == "typeop" or k == "jump":
        return 2
    if k == "varint":
        return 1 + len(varint_encode(ins[1]))
    if k == "multi":
        return 2 + sum(len(varint_encode(v)) for v in ins[1])
    if k == "stp":
        return 3
    if k == "raw":
        return len(ins[1])
    return 1   # op, end


def decode_script(rom, o, limit=8192):
    """Decode one script at file offset o. Returns [(file offset, ins)], the end offset. Decoding
    continues past an END when a forward jump targets beyond it. Non-canonical varints come back as
    ("raw", bytes) so the bytes round-trip."""
    out = []
    pc = o
    reach = o     # furthest forward jump target seen
    while pc - o < limit:
        op = rom[pc]
        at = pc
        if op == 0x00:
            ins, pc = ("byte", rom[pc + 1]), pc + 2
        elif op == 0x01:
            v, raw = varint_decode(rom, pc + 1)
            ins = ("varint", v) if varint_encode(v) == raw else ("raw", bytes([op]) + raw)
            pc += 1 + len(raw)
        elif op == 0x02:
            ins, pc = ("step", rom[pc + 1]), pc + 2
        elif op == 0x03:
            ins, pc = ("typeop", rom[pc + 1]), pc + 2
        elif op in JUMP_OPS:
            d = rom[pc + 1]
            tgt = pc + 2 + d if op != 0x1C else pc + 2 - d
            ins, pc = ("jump", op, tgt), pc + 2
            if op != 0x1C:
                reach = max(reach, tgt)
        elif op in STP_OPS:
            ins, pc = ("stp", op, rom[pc + 1], rom[pc + 2]), pc + 3
        elif op == 0x1D:
            n = rom[pc + 1]
            p = pc + 2
            vals, canon = [], True
            for _ in range(n):
                v, raw = varint_decode(rom, p)
                canon &= varint_encode(v) == raw
                vals.append(v)
                p += len(raw)
            ins = ("multi", vals) if canon else ("raw", bytes(rom[pc:p]))
            pc = p
        elif op == 0x11:
            out.append((at, ("end",)))
            pc += 1
            if reach < pc:
                return out, pc
            continue
        elif op in SIMPLE_OPS:
            ins, pc = ("op", op), pc + 1
        else:
            raise AsmError(f"unknown main opcode {op:#04x} at {ROM_BASE + pc:08x}")
        out.append((at, ins))
    raise AsmError(f"no END within {limit} bytes of {ROM_BASE + o:08x}")


def step_name(k):
    return STEP_DISPLAY[k] if k < len(STEP_DISPLAY) else str(k)


def macro_push(args):
    """The push the macro form emits for these arguments."""
    if len(args) == 1:
        return ("byte", args[0]) if 0 <= args[0] <= 255 else ("varint", args[0])
    return ("multi", list(args))


def push_args(ins):
    if ins[0] in ("byte", "varint"):
        return [ins[1]]
    if ins[0] == "multi":
        return ins[1]
    return None


def format_script(decoded, indent="    "):
    """Text lines for a decoded script ([(offset, ins)])."""
    targets = sorted({ins[2] for _, ins in decoded if ins[0] == "jump"})
    labels = {t: f"L{i + 1}" for i, t in enumerate(targets)}
    lines = []
    i = 0
    while i < len(decoded):
        at, ins = decoded[i]
        if at in labels:
            lines.append(f"{indent[:-2]}{labels[at]}:")
        nxt = decoded[i + 1] if i + 1 < len(decoded) else None
        args = push_args(ins)
        if (args is not None and nxt and nxt[1][0] == "step" and nxt[0] not in labels
                and macro_push(args) == ins and nxt[1][1] < len(STEP_DISPLAY)):
            lines.append(f"{indent}{step_name(nxt[1][1])} {', '.join(str(a) for a in args)}")
            i += 2
            continue
        lines.append(indent + format_ins(ins, labels))
        i += 1
    # a label at the very end (jump to the byte after the last END) cannot occur: decoding stops at
    # the END that covers every forward target
    return lines


def format_ins(ins, labels):
    k = ins[0]
    if k == "byte":
        return f"PushByte {ins[1]}"
    if k == "varint":
        return f"PushVarint {ins[1]}"
    if k == "multi":
        return "PushMulti " + ", ".join(str(v) for v in ins[1])
    if k == "step":
        return step_name(ins[1]) if ins[1] < len(STEP_DISPLAY) else f"Step {ins[1]}"
    if k == "typeop":
        return f"TypeOp {ins[1]}"
    if k == "op":
        return MAIN_OPS[ins[1]]
    if k == "jump":
        return f"{MAIN_OPS[ins[1]]} {labels[ins[2]]}"
    if k == "stp":
        return f"{MAIN_OPS[ins[1]]} {step_name(ins[2])}, {ins[3]}"
    if k == "raw":
        return f".bytes {ins[1].hex()}"
    return "END"


def parse_int(tok):
    tok = tok.strip()
    try:
        return int(tok, 0)
    except ValueError:
        raise AsmError(f"not a number: {tok!r}")


def split_args(rest):
    rest = rest.strip()
    if not rest:
        return []
    return [t for t in re.split(r"[,\s]+", rest) if t]


def parse_script_lines(lines):
    """Parse instruction lines into [ins] with ("label", name) markers."""
    out = []
    for ln, raw in lines:
        s = raw.strip()
        if not s:
            continue
        m = re.match(r"^([A-Za-z_][\w.]*):\s*(.*)$", s)
        if m and m.group(1) not in OP_BY_NAME:
            out.append(("label", m.group(1), ln))
            s = m.group(2).strip()
            if not s:
                continue
        parts = s.split(None, 1)
        mn, rest = parts[0], parts[1] if len(parts) > 1 else ""
        try:
            out.append(parse_ins(mn, rest))
        except AsmError as e:
            raise AsmError(f"line {ln}: {e}") from None
    return out


def parse_step(tok):
    if tok in STEP_BY_NAME:
        return STEP_BY_NAME[tok]
    try:
        k = int(tok, 0)
    except ValueError:
        raise AsmError(f"unknown step {tok!r}")
    if not 0 <= k <= 255:
        raise AsmError(f"step {k} out of range")
    return k


def parse_ins(mn, rest):
    args = split_args(rest)
    if mn == "PushByte":
        v = parse_int(args[0])
        if not 0 <= v <= 255:
            raise AsmError(f"PushByte {v} out of range (use PushVarint)")
        return ("byte", v)
    if mn == "PushVarint":
        return ("varint", parse_int(args[0]))
    if mn in ("PushMulti", "PushMultiVarint"):
        vals = [parse_int(a) for a in args]
        if not 1 <= len(vals) <= 255:
            raise AsmError("PushMulti takes 1..255 values")
        return ("multi", vals)
    if mn == "Step":
        return ("step", parse_step(args[0]))
    if mn == "TypeOp":
        return ("typeop", parse_int(args[0]))
    if mn == "END":
        return ("end",)
    if mn == ".bytes":
        return ("raw", bytes.fromhex("".join(args)))
    if mn in OP_BY_NAME:
        op = OP_BY_NAME[mn]
        if op in JUMP_OPS:
            return ("jump", op, args[0])
        if op in STP_OPS:
            return ("stp", op, parse_step(args[0]), parse_int(args[1]))
        if op in SIMPLE_OPS:
            if args:
                raise AsmError(f"{mn} takes no operand")
            return ("op", op)
        raise AsmError(f"{mn} needs its own syntax")
    if mn in STEP_BY_NAME or re.match(r"^Step_?\d+$", mn):
        k = STEP_BY_NAME[mn] if mn in STEP_BY_NAME else int(re.sub(r"\D", "", mn))
        vals = [parse_int(a) for a in args]
        return ("macro", k, vals)
    raise AsmError(f"unknown instruction {mn!r}")


def assemble_script(items, base=0):
    """[ins | ("label", ..) | ("macro", k, args)] -> bytes. Appends END when the last instruction
    is not END. base is the script's address (only used in error messages)."""
    flat = []
    for it in items:
        if it[0] == "macro":
            if it[2]:
                flat.append(macro_push(it[2]))
            flat.append(("step", it[1]))
        else:
            flat.append(it)
    real = [it for it in flat if it[0] != "label"]
    if not real or real[-1][0] != "end":
        flat.append(("end",))
    labels, pc = {}, 0
    for it in flat:
        if it[0] == "label":
            if it[1] in labels:
                raise AsmError(f"label {it[1]} defined twice")
            labels[it[1]] = pc
        else:
            pc += ins_size(it)
    out = bytearray()
    for it in flat:
        k = it[0]
        if k == "label":
            continue
        if k == "byte":
            out += bytes([0x00, it[1]])
        elif k == "varint":
            out += b"\x01" + varint_encode(it[1])
        elif k == "multi":
            out += bytes([0x1D, len(it[1])]) + b"".join(varint_encode(v) for v in it[1])
        elif k == "step":
            out += bytes([0x02, it[1]])
        elif k == "typeop":
            out += bytes([0x03, it[1]])
        elif k == "op":
            out.append(it[1])
        elif k == "stp":
            out += bytes([it[1], it[2], it[3]])
        elif k == "raw":
            out += it[1]
        elif k == "end":
            out.append(0x11)
        elif k == "jump":
            tgt = it[2]
            if isinstance(tgt, int):
                raise AsmError("jump target must be a label")
            if tgt not in labels:
                raise AsmError(f"undefined label {tgt}")
            here = len(out) + 2
            d = labels[tgt] - here if it[1] != 0x1C else here - labels[tgt]
            if not 0 <= d <= 255:
                raise AsmError(f"{MAIN_OPS[it[1]]} {tgt}: distance {d} out of range (forward 0..255"
                               f"{', LoopOrJump backward' if it[1] == 0x1C else ''})")
            out += bytes([it[1], d])
    return bytes(out)


# ---- text strings --------------------------------------------------------------------------------

def quote(b):
    out = []
    for c in b:
        if c == 0x22:
            out.append('\\"')
        elif c == 0x5C:
            out.append("\\\\")
        elif 32 <= c < 127:
            out.append(chr(c))
        else:
            out.append(f"\\x{c:02x}")
    return '"' + "".join(out) + '"'


def unquote(s):
    assert s[0] == '"' and s[-1] == '"'
    s = s[1:-1]
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\":
            n = s[i + 1]
            if n == "x":
                out.append(int(s[i + 2:i + 4], 16))
                i += 4
                continue
            if n in '"\\':
                out.append(ord(n))
            elif n == "n":
                out.append(0x0A)
            else:
                raise AsmError(f"unknown escape \\{n}")
            i += 2
            continue
        out += c.encode("latin-1")
        i += 1
    return bytes(out)


# ---- dialogue entries ----------------------------------------------------------------------------

def cstr(rom, o):
    return bytes(rom[o:rom.index(b"\0", o)])


def jcalg_text(rom, o):
    """(decompressed text up to its NUL, end offset of the stream)."""
    data, end = decompress(rom, o, 8, want_end=True)
    z = data.find(b"\0")
    return (data if z < 0 else data[:z]), end


def decode_entry(rom, addr):
    """A dict describing the entry at ROM address addr, with "size" = its bytes (not the separate
    format string of modes 3/4)."""
    e = off(addr)
    nxt, mode = struct.unpack_from("<HB", rom, e)
    d = {"addr": addr, "next": nxt, "mode": mode}
    if mode == 0:
        dec, end = decode_script(rom, e + 3)
        d.update(kind="script", script=dec, size=end - e)
    elif mode == 1:
        t = cstr(rom, e + 4)
        d.update(kind="text", speaker=rom[e + 3], text=t, size=4 + len(t) + 1)
    elif mode == 2:
        t, end = jcalg_text(rom, e + 4)
        d.update(kind="textz", speaker=rom[e + 3], text=t, size=end - e)
    elif mode == 3:
        fmt = struct.unpack_from("<I", rom, e + 4)[0]
        dec, end = decode_script(rom, e + 8)
        d.update(kind="fmt", speaker=rom[e + 3], fmt=fmt, text=cstr(rom, off(fmt)), script=dec, size=end - e)
    elif mode == 4:
        args = struct.unpack_from("<I", rom, e + 4)[0]
        t, end = jcalg_text(rom, e + 8)
        dec, _ = decode_script(rom, off(args))
        d.update(kind="fmtz", speaker=rom[e + 3], args=args, text=t, script=dec, size=end - e)
    elif mode == 5:
        d.update(kind="jump", pad=rom[e + 3], target=struct.unpack_from("<I", rom, e + 4)[0], size=8)
    else:
        raise AsmError(f"entry {addr:08x}: unknown mode {mode}")
    return d


def read_sequence(rom, seq):
    """[entry addresses], terminator word."""
    o = off(seq)
    ptrs = []
    while True:
        p = struct.unpack_from("<I", rom, o + 4 * len(ptrs))[0]
        if p >> 16 == 0:
            return ptrs, p
        if not is_rom(p):
            raise AsmError(f"sequence {seq:08x}[{len(ptrs)}] = {p:08x} is not an entry pointer")
        ptrs.append(p)
        if len(ptrs) > 1000:
            raise AsmError(f"sequence {seq:08x}: no terminator")


def format_entry(d, i, indent="  "):
    nxt = "" if d["next"] == i + 1 else f" -> {d['next']}"
    at = f" @{d['addr']:08x}"
    k = d["kind"]
    if k == "script":
        head = f"[{i}] script{nxt}{at}"
    elif k in ("text", "textz"):
        head = f"[{i}] {k} {d['speaker']}{nxt}{at} {quote(d['text'])}"
    elif k == "fmt":
        head = f"[{i}] fmt {d['speaker']}{nxt}{at} fmt=@{d['fmt']:08x} {quote(d['text'])}"
    elif k == "fmtz":
        head = f"[{i}] fmtz {d['speaker']}{nxt}{at} args=@{d['args']:08x} {quote(d['text'])}"
    else:
        pad = f" pad={d['pad']}" if d["pad"] else ""
        head = f"[{i}] jump {d['target']:#x}{pad}{nxt}{at}"
    lines = [indent + head]
    if k in ("script", "fmt", "fmtz"):
        lines += format_script(d["script"], indent + "      ")
    return lines


def format_sequence(rom, seq, name):
    ptrs, term = read_sequence(rom, seq)
    lines = [f"sequence {name} @{seq:08x}" + (f" end={term:#x}" if term else "")]
    for i, p in enumerate(ptrs):
        lines += format_entry(decode_entry(rom, p), i)
    lines.append("end")
    return lines


def format_script_block(rom, addr, name):
    dec, _ = decode_script(rom, off(addr))
    return [f"script {name} @{addr:08x}"] + format_script(dec) + ["end"]


# ---- parsing lscript files -----------------------------------------------------------------------

STR_RE = re.compile(r'"(?:[^"\\]|\\.)*"')


def strip_comment(line):
    """Drop ';' comments outside string literals."""
    out, i = [], 0
    while i < len(line):
        c = line[i]
        if c == '"':
            m = STR_RE.match(line, i)
            if not m:
                raise AsmError(f"unterminated string: {line.strip()!r}")
            out.append(m.group(0))
            i = m.end()
            continue
        if c == ";":
            break
        out.append(c)
        i += 1
    return "".join(out).rstrip()


def parse_attrs(tokens):
    """Split header tokens into positional args and key=value / @ADDR / -> N attributes."""
    pos, attrs = [], {}
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t == "->":
            attrs["next"] = parse_int(tokens[i + 1])
            i += 2
            continue
        if t.startswith("->"):
            attrs["next"] = parse_int(t[2:])
        elif t.startswith("@"):
            attrs["addr"] = int(t[1:], 16)
        elif "=" in t:
            k, v = t.split("=", 1)
            attrs[k] = v
        else:
            pos.append(t)
        i += 1
    return pos, attrs


def parse_file(text, path="<text>", rom=None):
    """Parse lscript text -> {name: block}. Blocks: {"type": "script", "items", "addr"} or
    {"type": "sequence", "entries": [...], "addr", "end"}. rom is needed for stockseq entries."""
    blocks = {}
    lines = [(n + 1, strip_comment(l)) for n, l in enumerate(text.splitlines())]
    i = 0

    def err(ln, msg):
        return AsmError(f"{path}:{ln}: {msg}")

    while i < len(lines):
        ln, s = lines[i]
        i += 1
        if not s.strip():
            continue
        head = s.split()
        if head[0] not in ("script", "sequence") or len(head) < 2:
            raise err(ln, f"expected 'script NAME' or 'sequence NAME', got {s.strip()!r}")
        kind, name = head[0], head[1]
        if name in blocks:
            raise err(ln, f"{name} defined twice")
        _, attrs = parse_attrs(head[2:])
        body = []
        while True:
            if i >= len(lines):
                raise err(ln, f"{kind} {name}: missing 'end'")
            bl, bs = lines[i]
            i += 1
            if bs.strip() == "end":
                break
            body.append((bl, bs))
        try:
            if kind == "script":
                blocks[name] = {"type": "script", "items": parse_script_lines(body), "addr": attrs.get("addr"),
                                "line": ln}
            else:
                blocks[name] = {"type": "sequence", "entries": parse_entries(body, rom),
                                "addr": attrs.get("addr"), "end": parse_int(attrs.get("end", "0")), "line": ln}
        except AsmError as e:
            raise AsmError(f"{path}: {kind} {name}: {e}") from None
    return blocks


def parse_entries(body, rom=None):
    entries = []
    cur = None
    for ln, s in body:
        st = s.strip()
        if not st:
            continue
        m = re.match(r"^(?:\[(\d*)\]\s*)?(script|text|textz|fmt|fmtz|jump|stockseq|stock)\b(.*)$", st)
        if m:
            idx, kind, rest = m.group(1), m.group(2), m.group(3)
            if idx not in (None, "") and int(idx) != len(entries):
                raise AsmError(f"line {ln}: entry [{idx}] is entry {len(entries)} of the sequence")
            if kind == "stockseq":       # stockseq @SEQ FROM [TO]: stock entries FROM..TO, next indexes shifted
                pos, attrs = parse_attrs(rest.split())
                if rom is None:
                    raise AsmError(f"line {ln}: stockseq needs the ROM")
                ptrs, _ = read_sequence(rom, attrs["addr"])
                lo = parse_int(pos[0]) if pos else 0
                hi = parse_int(pos[1]) if len(pos) > 1 else len(ptrs) - 1
                shift = len(entries) - lo
                for k in range(lo, hi + 1):
                    st_e = decode_entry(rom, ptrs[k])
                    entries.append({"kind": "stock", "line": ln, "text": None, "pos": [], "body": [],
                                    "attrs": {"addr": ptrs[k], "next": st_e["next"] + shift}})
                cur = None
                continue
            sm = STR_RE.search(rest)
            txt = unquote(sm.group(0)) if sm else None
            if sm:
                rest = rest[:sm.start()] + rest[sm.end():]
            pos, attrs = parse_attrs(rest.split())
            cur = {"kind": kind, "line": ln, "text": txt, "attrs": attrs, "pos": pos, "body": []}
            if kind in ("text", "textz", "fmt", "fmtz"):
                if not pos:
                    raise AsmError(f"line {ln}: {kind} needs a speaker")
                cur["speaker"] = parse_int(pos[0])
                if txt is None:
                    raise AsmError(f"line {ln}: {kind} needs a text string")
            if kind == "jump":
                cur["target"] = parse_int(pos[0])
            entries.append(cur)
            continue
        if cur is None or cur["kind"] not in ("script", "fmt", "fmtz"):
            raise AsmError(f"line {ln}: instruction outside a script entry: {st!r}")
        # (a stock entry takes no instructions: its bytes are the ROM's)
        cur["body"].append((ln, s))
    for i, e in enumerate(entries):
        e["next"] = e["attrs"].get("next", i + 1)
        e["addr"] = e["attrs"].get("addr")
        if e["kind"] in ("script", "fmt", "fmtz"):
            e["items"] = parse_script_lines(e["body"])
        if "fmt" in e["attrs"]:
            e["fmt_addr"] = int(e["attrs"]["fmt"].lstrip("@"), 16)
        if "args" in e["attrs"]:
            e["args_addr"] = int(e["attrs"]["args"].lstrip("@"), 16)
    return entries


# ---- assembling entries and sequences ------------------------------------------------------------

class Placer:
    """Hands out ROM space from a free file offset and collects the bytes as patch records. With
    rom given, items whose bytes equal the stock bytes at their @ADDR are not written (reuse)."""

    def __init__(self, free, rom=None, limit=None):
        self.free = free
        self.start = free
        self.limit = limit
        self.rom = rom
        self.records = []

    def put(self, data, align=1, note=""):
        self.free = (self.free + align - 1) & ~(align - 1)
        at = self.free
        if self.limit is not None and at + len(data) > self.limit:
            raise AsmError(f"free space exhausted at {at:#x} (limit {self.limit:#x})")
        self.records.append({"offset": f"{at:#x}", "bytes": bytes(data).hex(), "note": note})
        self.free += len(data)
        return ROM_BASE + at

    def stock_equal(self, addr, data):
        if self.rom is None or addr is None:
            return False
        o = off(addr)
        return 0 <= o and o + len(data) <= len(self.rom) and self.rom[o:o + len(data)] == data


def stock_entry(rom, addr):
    try:
        return decode_entry(rom, addr) if rom is not None and addr is not None else None
    except Exception:  # noqa: BLE001
        return None


def stock_fmtz(e, rom):
    """The stock mode-4 entry at the entry's @ADDR when its text and speaker are unchanged."""
    st = stock_entry(rom, e["addr"]) if e["kind"] == "fmtz" else None
    if st and st["mode"] == 4 and st["text"] == e["text"] and st["speaker"] == e["speaker"]:
        return st
    return None


def entry_bytes(e, rom, ptr=None):
    """Bytes of one parsed entry -> (bytes, align, mode). ptr: mode 3's format string pointer, or for
    an unchanged fmtz entry (kept as mode 4) the pointer to its argument script."""
    k = e["kind"]
    nxt = e["next"]
    if not 0 <= nxt <= 0xFFFF:
        raise AsmError(f"line {e['line']}: next index {nxt} out of range")
    if k == "script":
        return struct.pack("<HB", nxt, 0) + assemble_script(e["items"]), 2, 0
    if k == "text":
        return struct.pack("<HBB", nxt, 1, e["speaker"]) + e["text"] + b"\0", 2, 1
    if k == "textz":
        st = stock_entry(rom, e["addr"])
        if st and st["mode"] == 2 and st["text"] == e["text"] and st["speaker"] == e["speaker"]:
            o = off(e["addr"])
            data = bytearray(rom[o:o + st["size"]])
            struct.pack_into("<H", data, 0, nxt)
            return bytes(data), 4, 2
        return struct.pack("<HBB", nxt, 1, e["speaker"]) + e["text"] + b"\0", 2, 1
    if k == "fmtz" and stock_fmtz(e, rom):
        st = stock_fmtz(e, rom)
        o = off(e["addr"])
        data = bytearray(rom[o:o + st["size"]])
        struct.pack_into("<HBBI", data, 0, nxt, 4, e["speaker"], ptr)
        return bytes(data), 4, 4
    if k in ("fmt", "fmtz"):    # an edited fmtz becomes mode 3 (plain format string, inline script)
        return struct.pack("<HBBI", nxt, 3, e["speaker"], ptr or 0) + assemble_script(e["items"]), 4, 3
    if k == "jump":
        return struct.pack("<HBBI", nxt, 5, parse_int(e["attrs"].get("pad", "0")), e["target"]), 4, 5
    if k == "stock":    # the ROM's entry at @ADDR as it is, with this sequence's next index
        st = stock_entry(rom, e["addr"])
        if st is None:
            raise AsmError(f"line {e['line']}: no stock entry at {e['addr']}")
        o = off(e["addr"])
        data = bytearray(rom[o:o + st["size"]])
        struct.pack_into("<H", data, 0, nxt)
        return bytes(data), 4, st["mode"]
    raise AsmError(f"unknown entry kind {k}")


def entry_pointer(e, rom, placer, name):
    """The u32 at +4 of a mode 3/4 entry: the stock format string when the text is unchanged (else
    a new string), or for a kept mode-4 entry its argument script (stock when unchanged)."""
    if e["kind"] == "fmtz" and stock_fmtz(e, rom):
        st = stock_fmtz(e, rom)
        script = assemble_script(e["items"])
        if placer.stock_equal(st["args"], script):
            return st["args"]
        return placer.put(script, 1, f"{name}: format arguments script")
    fa = e.get("fmt_addr")
    if rom is not None and fa is not None and e["kind"] == "fmt":
        try:
            if cstr(rom, off(fa)) == e["text"]:
                return fa
        except Exception:  # noqa: BLE001
            pass
    return placer.put(e["text"] + b"\0", 1, f"{name}: format string {e['text'][:40]!r}")


def place_entry(e, rom, placer, name):
    ptr = entry_pointer(e, rom, placer, name) if e["kind"] in ("fmt", "fmtz") else None
    data, align, mode = entry_bytes(e, rom, ptr)
    if placer.stock_equal(e["addr"], data):
        return e["addr"]
    label = e["text"][:40] if e.get("text") else e["kind"]
    return placer.put(data, align, f"{name}: entry mode {mode} {label!r}")


def place_block(name, block, rom, placer):
    """Assemble one parsed block into placer; returns its ROM address."""
    if block["type"] == "script":
        data = assemble_script(block["items"])
        if placer.stock_equal(block["addr"], data):
            return block["addr"]
        return placer.put(data, 1, f"script {name}")
    ptrs = [place_entry(e, rom, placer, name) for e in block["entries"]]
    arr = b"".join(struct.pack("<I", p) for p in ptrs) + struct.pack("<I", block["end"])
    if placer.stock_equal(block["addr"], arr):
        return block["addr"]
    return placer.put(arr, 4, f"sequence {name} ({len(ptrs)} entries)")


def assemble_file(blocks, rom, placer):
    """Place every block; returns {name: address}."""
    return {name: place_block(name, b, rom, placer) for name, b in blocks.items()}


# ---- corpus (every script and sequence the ROM reaches) ------------------------------------------

def walk_corpus(rom):
    """(scripts {addr: where}, sequences {addr: where}) reachable from the map table, items, objects
    and the quest table."""
    scripts, seqs = {}, {}
    w = lambda a: struct.unpack_from("<I", rom, off(a))[0]

    def add(table, addr, where):
        if is_rom(addr):
            table.setdefault(addr, where)

    def cond(word, where):
        if is_rom(word):
            add(scripts, word, where + " condition")

    def payload(p, where, hint=None):
        if not is_rom(p):
            return
        if hint == "seq" or (hint is None and is_rom(w(p))):
            add(seqs, p, where)
        else:
            add(scripts, p, where)

    for i in range(MAP_COUNT):
        m = map_entry(rom, i)
        tag = f"map {i} (z{m['zone']} a{m['area']})"
        for label, k in (("name", 0xC), ("music", 0x24), ("variation", 0x28), ("on-load", 0x30), ("on-leave", 0x34)):
            v = w(m["addr"] + k)
            if is_rom(v):
                add(scripts, v, f"{tag} {label}")
        for t in range(m["triggers"]):
            c, data = struct.unpack_from("<II", rom, off(m["trig_ptr"]) + 8 * t)
            cond(c, f"{tag} trigger {t}")
            cfn = w(data)
            if cfn == TRIGGER_SAVE_COND:
                continue
            p = w(data + 0x10)
            payload(p, f"{tag} trigger {t}",
                    "seq" if cfn == TRIGGER_DIALOG_COND else "script" if cfn == TRIGGER_SCRIPT_COND else None)
        for s in range(m["scripts"]):
            r = w(m["script_ptr"] + 4 * s)
            h = w(r)
            cond(w(r + 4), f"{tag} record {s}")
            if h == HANDLER_NPC:
                payload(w(r + 0xC), f"{tag} record {s} dialogue", "seq")
            elif h == HANDLER_ENEMY:
                act = w(r + 0x14)
                if is_rom(act):
                    fn, p = w(act), w(act + 4)
                    if fn == ACTION_DIALOG:
                        payload(p, f"{tag} record {s} defeat", "seq")
                    elif fn == ACTION_SCRIPT:
                        payload(p, f"{tag} record {s} defeat", "script")
        for k in range(m["items"]):
            r = w(m["item_ptr"] + 4 * k)
            cond(w(r), f"{tag} item {k}")
            if w(r + 8) in ITEM_FNS:
                payload(w(r + 0xC), f"{tag} item {k}")   # a pickup script or a dialogue sequence
        for k in range(m["objects"]):
            r = w(m["object_ptr"] + 4 * k)
            cond(w(r), f"{tag} object {k}")
            if w(r + 0x10) == OBJECT_FN:
                payload(w(r + 0x14), f"{tag} object {k} dialogue", "seq")
    for q in range(QUEST_COUNT):
        _, avail, done, _ = struct.unpack_from("<IIII", rom, off(QUEST_TABLE) + 16 * q)
        add(scripts, avail, f"quest {q} available")
        add(scripts, done, f"quest {q} complete")
    return scripts, seqs


def roundtrip(rom, verbose=False):
    """Dump -> parse -> assemble every stock script and sequence; compare with the ROM bytes."""
    scripts, seqs = walk_corpus(rom)
    fails = 0
    n_entries = 0
    modes = {}
    for addr, where in sorted(scripts.items()):
        try:
            text = "\n".join(format_script_block(rom, addr, "s"))
            blk = parse_file(text)["s"]
            data = assemble_script(blk["items"])
            _, end = decode_script(rom, off(addr))
            want = bytes(rom[off(addr):end])
            if data != want:
                raise AsmError(f"bytes differ:\n  rom {want.hex()}\n  asm {data.hex()}")
        except AsmError as e:
            fails += 1
            print(f"FAIL script {addr:08x} ({where}): {e}")
    for addr, where in sorted(seqs.items()):
        try:
            text = "\n".join(format_sequence(rom, addr, "q"))
            blk = parse_file(text)["q"]
            ptrs, term = read_sequence(rom, addr)
            if len(blk["entries"]) != len(ptrs) or blk["end"] != term:
                raise AsmError("entry count or terminator differs")
            for e, p in zip(blk["entries"], ptrs):
                st = decode_entry(rom, p)
                ptr = None
                if e["kind"] == "fmt":
                    # the format string pointer is the stock one only when the text round-trips
                    if cstr(rom, off(st["fmt"])) != e["text"] or e.get("fmt_addr") != st["fmt"]:
                        raise AsmError(f"entry {p:08x}: format text does not round-trip")
                    ptr = st["fmt"]
                elif e["kind"] == "fmtz":
                    script = assemble_script(e["items"])
                    _, end = decode_script(rom, off(st["args"]))
                    if script != rom[off(st["args"]):end] or e.get("args_addr") != st["args"]:
                        raise AsmError(f"entry {p:08x}: argument script does not round-trip")
                    ptr = st["args"]
                data, _, _ = entry_bytes(e, rom, ptr)
                want = bytes(rom[off(p):off(p) + st["size"]])
                if data != want:
                    raise AsmError(f"entry {p:08x} bytes differ:\n  rom {want.hex()}\n  asm {data.hex()}")
                n_entries += 1
                modes[st["mode"]] = modes.get(st["mode"], 0) + 1
        except AsmError as e:
            fails += 1
            print(f"FAIL sequence {addr:08x} ({where}): {e}")
    print(f"round trip: {len(scripts)} scripts, {len(seqs)} sequences, {n_entries} entries "
          f"(modes {dict(sorted(modes.items()))}), {fails} failures")
    return fails


# ---- map dump ------------------------------------------------------------------------------------

def find_map(rom, zone, area, which=0):
    matches = [i for i in range(MAP_COUNT) if rom[off(MAP_ENTRIES) + i * MAP_ENTRY_SIZE] == zone
               and rom[off(MAP_ENTRIES) + i * MAP_ENTRY_SIZE + 1] == area]
    if not matches:
        raise AsmError(f"no map entry for zone {zone} area {area}")
    return matches[min(which, len(matches) - 1)]


def dump_map(rom, idx):
    """lscript text for every script and sequence of map idx, named m<idx>_<where>."""
    m = map_entry(rom, idx)
    w = lambda a: struct.unpack_from("<I", rom, off(a))[0]
    pre = f"m{idx}"
    out = [f"; map {idx} @ {m['addr']:08x}: zone {m['zone']} area {m['area']}, {m['triggers']} triggers, "
           f"{m['scripts']} records (tools/script_asm.py dump map {m['zone']} {m['area']})", ""]
    seen = set()

    def emit(addr, name, seq):
        if not is_rom(addr) or addr in seen:
            if addr in seen:
                out.append(f"; {name} = {addr:08x} (dumped above)\n")
            return
        seen.add(addr)
        out.extend(format_sequence(rom, addr, name) if seq else format_script_block(rom, addr, name))
        out.append("")

    for label, k in (("name", 0xC), ("music", 0x24), ("variation", 0x28), ("onload", 0x30), ("onleave", 0x34)):
        emit(w(m["addr"] + k), f"{pre}_{label}", False)
    for t in range(m["triggers"]):
        c, data = struct.unpack_from("<II", rom, off(m["trig_ptr"]) + 8 * t)
        cfn, fn, x1, y1, x2, y2 = struct.unpack_from("<IIhhhh", rom, off(data))
        out.append(f"; trigger {t} @ {data:08x}: condition word {c:#x}, cond fn {cfn:08x}, fn {fn:08x}, "
                   f"rect ({x1},{y1})-({x2},{y2})")
        if is_rom(c):
            emit(c, f"{pre}_t{t}_cond", False)
        if cfn != TRIGGER_SAVE_COND:
            p = w(data + 0x10)
            emit(p, f"{pre}_t{t}", cfn == TRIGGER_DIALOG_COND or (cfn != TRIGGER_SCRIPT_COND and is_rom(p) and is_rom(w(p))))
    for s in range(m["scripts"]):
        r = w(m["script_ptr"] + 4 * s)
        h, c = w(r), w(r + 4)
        if h == HANDLER_NPC:
            x, y, dlg, spr = struct.unpack_from("<hhII", rom, off(r) + 8)
            out.append(f"; record {s} @ {r:08x}: NPC sprite {spr} at ({x},{y}) condition {c:#x}")
            emit(dlg, f"{pre}_r{s}_talk", True)
        elif h == HANDLER_ACTOR:
            x, y, spr = struct.unpack_from("<hhI", rom, off(r) + 8)
            out.append(f"; record {s} @ {r:08x}: actor sprite {spr} at ({x},{y}) condition {c:#x}")
        elif h == HANDLER_ENEMY:
            stat, x, y, spr, act = struct.unpack_from("<IhhII", rom, off(r) + 8)
            out.append(f"; record {s} @ {r:08x}: enemy stat {stat} sprite {spr} at ({x},{y}) condition {c:#x}")
            if is_rom(act):
                fn, p = w(act), w(act + 4)
                if fn in (ACTION_DIALOG, ACTION_SCRIPT):
                    emit(p, f"{pre}_r{s}_defeat", fn == ACTION_DIALOG)
        else:
            out.append(f"; record {s} @ {r:08x}: handler {h:08x}")
        if is_rom(c):
            emit(c, f"{pre}_r{s}_cond", False)
    return "\n".join(out) + "\n"


def main():
    args = sys.argv[1:]
    rom_path = "roms/log2.gba"
    if "--rom" in args:
        i = args.index("--rom")
        rom_path = args[i + 1]
        del args[i:i + 2]
    if not args:
        sys.exit(__doc__)
    rom = open(rom_path, "rb").read()
    cmd = args[0]
    try:
        if cmd == "dump":
            what = args[1]
            if what == "map":
                which = 0
                if "--index" in args:
                    i = args.index("--index")
                    which = int(args[i + 1])
                    del args[i:i + 2]
                sys.stdout.write(dump_map(rom, find_map(rom, int(args[2]), int(args[3]), which)))
            elif what in ("script", "seq"):
                addr = int(args[2], 16) if not args[2].startswith("0x") else int(args[2], 0)
                name = args[3] if len(args) > 3 else f"{what}_{addr:08x}"
                lines = format_script_block(rom, addr, name) if what == "script" else format_sequence(rom, addr, name)
                print("\n".join(lines))
            else:
                sys.exit(__doc__)
        elif cmd == "asm":
            path = args[1]
            i = args.index("--free")
            free = int(args[i + 1], 0)
            out = args[args.index("--out") + 1] if "--out" in args else None
            blocks = parse_file(open(path).read(), path)
            placer = Placer(free, rom)
            names = assemble_file(blocks, rom, placer)
            for n, a in names.items():
                print(f"{n:30} {a:08x}")
            print(f"{len(placer.records)} records, free {placer.start:#x} -> {placer.free:#x}")
            if out:
                json.dump(placer.records, open(out, "w"), indent=1)
        elif cmd == "roundtrip":
            sys.exit(1 if roundtrip(rom, "-v" in args) else 0)
        else:
            sys.exit(__doc__)
    except AsmError as e:
        sys.exit(f"error: {e}")


if __name__ == "__main__":
    main()
