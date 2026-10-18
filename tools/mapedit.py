"""Map patcher for Legacy of Goku II (USA): applies a story spec (data/story/*.json) to the map table
and emits tools/patch_rom.py --data-patch records. tools/romdata.py runs it for every spec that
data/hercule.json includes; it can also run alone for checking a spec.

usage: mapedit.py build SPEC.json [SPEC.json ...] OUT.json [--rom ROM] [--free 0xB00000]
       mapedit.py show SPEC.json [--rom ROM]       assemble and print the records without writing

A spec (docs/scripts.md has the reference and examples):
{
  "_": "comment",
  "sources": ["data/story/a16_island.lscript"],   lscript files with named scripts and sequences
  "maps": [
    {
      "map": {"zone": 11, "area": 4},            or {"index": 296}; "which": N picks among duplicates
      "onload": "script:NAME",                   also "name", "music", "variation", "onleave"
      "triggers": {"0": {"payload": "seq:NAME", "rect": [x1, y1, x2, y2], "condition": COND}},
      "add_triggers": [{"like": 0, "rect": [..], "payload": "seq:NAME", "condition": COND}],
      "records": {"1": {"stat": 109, "sprite": 160, "action": "seq:NAME"},
                  "3": {"sprite": 9}, "5": "disable"},
      "add_records": [{"kind": "actor", "sprite": 76, "x": 400, "y": 300, "facing": 2, "condition": COND},
                      {"like": 4, "sprite": 72, "talk": "seq:NAME"}],
      "edits": [{"at": "trigger:3/entry:15", "replace": [["SetStoryFlag 145", "SetStoryFlag 146"]]}]
    }
  ]
}

References (REF): "seq:NAME" / "script:NAME" (a block of the spec's sources, assembled into free ROM
once; unchanged stock entries are reused), "@08xxxxxx" (an address), "none" (enemy action: nothing).
Conditions (COND): 0 / "always", "never" (the stock "spawned by a script" value 1), "flag:N" (story
flag N set), "noflag:N" (clear), "once:N" (set, and cleared when it fires), "script:NAME", or a number.
Records keep their indexes (scripts spawn records by index: TriggerMapEvent N,
DespawnCharacterAndRecordPosition [char, N]), so "disable" sets the condition to "never" instead of
deleting; new records are appended (the pointer array is copied to free ROM, the count updated).
Record fields: condition, x, y, sprite; enemy: stat, action (REF: a sequence = defeat dialogue, a
script = defeat script, "none"); NPC: talk (REF); actor/NPC/enemy built from scratch with "kind" get
one FaceAndWait behaviour ("facing": 0 down, 1 up, 2 left, 3 right).
Edit paths: "trigger:N", "record:N/defeat", "record:N/talk", "onload" (and the other map scripts),
with "/entry:K" for one entry of a sequence. "replace" pairs match whole instruction or entry lines
(whitespace-normalised, as tools/script_asm.py dump prints them); an edited script is reassembled
into free ROM and its pointer field repointed, an edited entry is re-placed and its sequence slot
repointed (sequences are shared, so the edit applies wherever the sequence is used).
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import script_asm as sa  # noqa: E402
from script_asm import AsmError, ROM_BASE, off, is_rom  # noqa: E402

STORY_FREE = 0xB00000          # file offset of the story data region in the 16 MiB output ROM
STORY_LIMIT = 0xC00000
ROM_SIZE = 0x01000000
FACE_AND_WAIT = 0x0800CB0F      # NpcBehavior_FaceAndWait_Create: {fn, flags: byte 2 facing, low 16 ticks}
WAIT_TICKS = 0x7D00
MAP_FIELDS = {"name": 0xC, "music": 0x24, "variation": 0x28, "onload": 0x30, "onleave": 0x34}


def num(v):
    return int(v, 0) if isinstance(v, str) else v


class MapEditor:
    """A working copy of the ROM that every edit is applied to (so later specs see earlier ones),
    plus the patch records that reproduce the edits."""

    def __init__(self, rom, free=STORY_FREE, limit=STORY_LIMIT):
        self.stock = bytes(rom)
        self.rom = bytearray(rom) + bytearray(max(0, ROM_SIZE - len(rom)))
        self.placer = sa.Placer(free, self.stock, limit)
        self.records = []
        self.blocks = {}       # name -> parsed block (all sources seen so far)
        self.placed = {}       # name -> address

    # -- low level --
    def w32(self, addr):
        return struct.unpack_from("<I", self.rom, off(addr))[0]

    def write(self, addr, data, note):
        o = off(addr)
        self.records.append({"offset": f"{o:#x}", "expect": bytes(self.rom[o:o + len(data)]).hex(),
                             "bytes": bytes(data).hex(), "note": note})
        self.rom[o:o + len(data)] = data

    def put(self, data, align, note):
        n = len(self.placer.records)
        addr = self.placer.put(data, align, note)
        self.sync(n)
        return addr

    def sync(self, n):
        """Move the placer's new records into ours and the working ROM."""
        for r in self.placer.records[n:]:
            o = int(r["offset"], 16)
            b = bytes.fromhex(r["bytes"])
            self.rom[o:o + len(b)] = b
            self.records.append(r)
        del self.placer.records[n:]

    # -- sources and references --
    def load_sources(self, paths):
        for p in paths:
            for name, blk in sa.parse_file(open(p).read(), p, self.stock).items():
                if name in self.blocks:
                    raise AsmError(f"{p}: {name} is already defined by another source")
                self.blocks[name] = blk

    def ref(self, v, want=None):
        """Address of a REF ("seq:NAME", "script:NAME", "@08xxxxxx")."""
        if isinstance(v, int):
            return v
        if v.startswith("@"):
            return int(v[1:], 16)
        if v.startswith("0x"):
            return int(v, 16)
        kind, _, name = v.partition(":")
        if kind not in ("seq", "script") or not name:
            raise AsmError(f"bad reference {v!r}")
        blk = self.blocks.get(name)
        if blk is None:
            raise AsmError(f"{v}: no block named {name} in the sources")
        if (kind == "seq") != (blk["type"] == "sequence"):
            raise AsmError(f"{v}: {name} is a {blk['type']}")
        if want and want != blk["type"]:
            raise AsmError(f"{v}: a {want} is needed here")
        if name not in self.placed:
            n = len(self.placer.records)
            self.placed[name] = sa.place_block(name, blk, self.stock, self.placer)
            self.sync(n)
        return self.placed[name]

    def ref_kind(self, v):
        if isinstance(v, str) and v.startswith("seq:"):
            return "sequence"
        if isinstance(v, str) and v.startswith("script:"):
            return "script"
        return None

    def condition(self, v):
        if isinstance(v, int):
            return v
        if v in ("always", "0"):
            return 0
        if v == "never":
            return 1
        kind, _, arg = v.partition(":")
        if kind == "flag":
            return 0x10000 | num(arg)
        if kind == "noflag":
            return 0x20000 | num(arg)
        if kind == "once":
            return 0x30000 | num(arg)
        if kind == "script":
            return self.ref(v, "script")
        return num(v)

    # -- maps --
    def map_index(self, m):
        if "index" in m:
            return num(m["index"])
        return sa.find_map(self.rom, num(m["zone"]), num(m["area"]), num(m.get("which", 0)))

    def entry(self, idx):
        return sa.MAP_ENTRIES + idx * sa.MAP_ENTRY_SIZE

    def apply(self, spec, path="<spec>"):
        self.load_sources(spec.get("sources", []))
        for m in spec.get("maps", []):
            idx = self.map_index(m["map"])
            tag = f"{os.path.basename(path)} map {idx}"
            try:
                self.apply_map(idx, m, tag)
            except AsmError as e:
                raise AsmError(f"{tag}: {e}") from None

    def apply_map(self, idx, m, tag):
        me = self.entry(idx)
        for field, k in MAP_FIELDS.items():
            if field in m:
                self.write(me + k, struct.pack("<I", self.ref(m[field], "script")), f"{tag} {field} script")
        for t, ch in m.get("triggers", {}).items():
            self.edit_trigger(me, int(t), ch, tag)
        for t in m.get("add_triggers", []):
            self.add_trigger(me, t, tag)
        for r, ch in m.get("records", {}).items():
            self.edit_record(me, int(r), ch, tag)
        for r in m.get("add_records", []):
            self.add_record(me, r, tag)
        for e in m.get("edits", []):
            self.edit_path(me, e["at"], e["replace"], tag)

    # -- triggers --
    def trigger(self, me, t):
        count = self.rom[off(me) + 3]
        if not 0 <= t < count:
            raise AsmError(f"trigger {t}: the map has {count}")
        slot = self.w32(me + 0x10) + 8 * t
        return slot, self.w32(slot + 4)

    def edit_trigger(self, me, t, ch, tag):
        slot, data = self.trigger(me, t)
        for k, v in ch.items():
            if k == "payload":
                self.write(data + 0x10, struct.pack("<I", self.ref(v)), f"{tag} trigger {t} payload {v}")
            elif k == "rect":
                self.write(data + 8, struct.pack("<4h", *[num(x) for x in v]), f"{tag} trigger {t} rect {v}")
            elif k == "condition":
                self.write(slot, struct.pack("<I", self.condition(v)), f"{tag} trigger {t} condition {v}")
            elif k != "_":
                raise AsmError(f"trigger {t}: unknown field {k!r}")

    def add_trigger(self, me, t, tag):
        count = self.rom[off(me) + 3]
        like_slot, like_data = self.trigger(me, num(t["like"]))
        cond = self.w32(like_slot)
        cfn, fn, x1, y1, x2, y2, payload = struct.unpack_from("<IIhhhhI", self.rom, off(like_data))
        rect = [num(x) for x in t.get("rect", [x1, y1, x2, y2])]
        if "payload" in t:
            payload = self.ref(t["payload"])
        if "condition" in t:
            cond = self.condition(t["condition"])
        data = self.put(struct.pack("<II4hI", cfn, fn, *rect, payload), 4, f"{tag} new trigger {count} data")
        arr = bytes(self.rom[off(self.w32(me + 0x10)):off(self.w32(me + 0x10)) + 8 * count])
        arr += struct.pack("<II", cond, data)
        new = self.put(arr, 4, f"{tag} trigger array ({count + 1})")
        self.write(me + 0x10, struct.pack("<I", new), f"{tag} trigger array -> {new:08x}")
        self.write(me + 3, bytes([count + 1]), f"{tag} trigger count {count + 1}")

    # -- records --
    def record(self, me, r):
        count = self.rom[off(me) + 4]
        if not 0 <= r < count:
            raise AsmError(f"record {r}: the map has {count}")
        return self.w32(self.w32(me + 0x14) + 4 * r)

    def record_layout(self, rec):
        h = self.w32(rec)
        if h == sa.HANDLER_NPC:
            return h, {"x": 8, "y": 0xA, "talk": 0xC, "sprite": 0x10}
        if h == sa.HANDLER_ACTOR:
            return h, {"x": 8, "y": 0xA, "sprite": 0xC}
        if h == sa.HANDLER_ENEMY:
            return h, {"stat": 8, "x": 0xC, "y": 0xE, "sprite": 0x10, "action": 0x14}
        raise AsmError(f"record at {rec:08x}: handler {h:08x} is not editable")

    def action_obj(self, v, tag):
        """A new {func, payload} enemy action object for a REF."""
        if v == "none":
            fn, p = sa.ACTION_NONE, 0
        else:
            p = self.ref(v)
            fn = sa.ACTION_SCRIPT if self.ref_kind(v) == "script" else sa.ACTION_DIALOG
            if self.ref_kind(v) is None:   # a raw address: a sequence starts with ROM pointers
                fn = sa.ACTION_DIALOG if is_rom(self.w32(p)) else sa.ACTION_SCRIPT
        return self.put(struct.pack("<II", fn, p), 4, f"{tag} action object {v}")

    def set_fields(self, rec, ch, tag, label):
        h, lay = self.record_layout(rec)
        for k, v in ch.items():
            if k in ("_", "like", "kind", "facing"):
                continue
            if k == "condition":
                self.write(rec + 4, struct.pack("<I", self.condition(v)), f"{tag} {label} condition {v}")
            elif k not in lay:
                raise AsmError(f"{label}: field {k!r} does not apply to this record kind")
            elif k in ("x", "y"):
                self.write(rec + lay[k], struct.pack("<h", num(v)), f"{tag} {label} {k} {v}")
            elif k == "talk":
                self.write(rec + lay[k], struct.pack("<I", self.ref(v, "sequence")), f"{tag} {label} talk {v}")
            elif k == "action":
                self.write(rec + lay[k], struct.pack("<I", self.action_obj(v, tag)), f"{tag} {label} action {v}")
            else:
                self.write(rec + lay[k], struct.pack("<I", num(v)), f"{tag} {label} {k} {v}")

    def edit_record(self, me, r, ch, tag):
        rec = self.record(me, r)
        if ch == "disable":
            ch = {"condition": "never"}
        self.set_fields(rec, ch, tag, f"record {r}")

    def add_record(self, me, spec, tag):
        count = self.rom[off(me) + 4]
        if "like" in spec:
            like = spec["like"]
            src_me = self.entry(self.map_index(like["map"])) if isinstance(like, dict) else me
            src = self.record(src_me, num(like["record"]) if isinstance(like, dict) else num(like))
            h, _ = self.record_layout(src)
            size = {sa.HANDLER_NPC: 0x18, sa.HANDLER_ACTOR: 0x10, sa.HANDLER_ENEMY: 0x1C}[h]
            n = self.w32(src + size - 4)       # behaviour count; the pointers follow
            data = bytes(self.rom[off(src):off(src) + size + 4 * n])
            rec = self.put(data, 4, f"{tag} new record {count} (copy of {src:08x})")
            self.set_fields(rec, spec, tag, f"record {count}")
        else:
            rec = self.new_record(spec, tag, count)
        arr = bytes(self.rom[off(self.w32(me + 0x14)):off(self.w32(me + 0x14)) + 4 * count])
        arr += struct.pack("<I", rec)
        new = self.put(arr, 4, f"{tag} record array ({count + 1})")
        self.write(me + 0x14, struct.pack("<I", new), f"{tag} record array -> {new:08x}")
        self.write(me + 4, bytes([count + 1]), f"{tag} record count {count + 1}")

    def new_record(self, spec, tag, count):
        kind = spec["kind"]
        x, y, sprite = num(spec.get("x", 0)), num(spec.get("y", 0)), num(spec["sprite"])
        cond = self.condition(spec.get("condition", 0))
        behaviour = struct.pack("<II", FACE_AND_WAIT, (num(spec.get("facing", 0)) << 16) | WAIT_TICKS)
        at = (self.placer.free + 3) & ~3
        base = ROM_BASE + at
        if kind == "actor":       # +0x10 count, +0x14 ptr -> the inline behaviour at +0x18
            data = struct.pack("<IIhhIII", sa.HANDLER_ACTOR, cond, x, y, sprite, 1, base + 0x18) + behaviour
        elif kind == "npc":       # +0xC dialogue, +0x10 sprite, +0x14 count, +0x18 ptr, behaviour at +0x1C
            talk = self.ref(spec["talk"], "sequence") if "talk" in spec else 0
            at = (self.placer.free + 3) & ~3
            base = ROM_BASE + at
            data = struct.pack("<IIhhIIII", sa.HANDLER_NPC, cond, x, y, talk, sprite, 1, base + 0x1C) + behaviour
        elif kind == "enemy":     # +0x14 action, +0x18 count, +0x1C ptr, behaviour at +0x20, action at +0x28
            act = spec.get("action", "none")
            if act == "none":
                fn, p = sa.ACTION_NONE, 0
            else:
                p = self.ref(act)
                fn = sa.ACTION_SCRIPT if self.ref_kind(act) == "script" else sa.ACTION_DIALOG
            at = (self.placer.free + 3) & ~3
            base = ROM_BASE + at
            data = struct.pack("<IIIhhIIII", sa.HANDLER_ENEMY, cond, num(spec["stat"]), x, y, sprite, base + 0x28, 1,
                               base + 0x20) + behaviour + struct.pack("<II", fn, p)
        else:
            raise AsmError(f"new record: unknown kind {kind!r} (actor, npc, enemy)")
        rec = self.put(data, 4, f"{tag} new {kind} record {count} sprite {sprite}")
        assert rec == base
        return rec

    # -- edits --
    def resolve(self, me, path):
        """Path -> (pointer field address, target address, kind, entry index or None)."""
        parts = path.split("/")
        head = parts[0]
        entry = None
        if len(parts) > 1 and parts[-1].startswith("entry:"):
            entry = int(parts[-1].split(":")[1])
            parts = parts[:-1]
        if head.startswith("trigger:"):
            t = int(head.split(":")[1])
            _, data = self.trigger(me, t)
            field = data + 0x10
            cfn = self.w32(data)
            target = self.w32(field)
            kind = "sequence" if cfn == sa.TRIGGER_DIALOG_COND else "script" if cfn == sa.TRIGGER_SCRIPT_COND else (
                "sequence" if is_rom(self.w32(target)) else "script")
        elif head.startswith("record:"):
            rec = self.record(me, int(head.split(":")[1]))
            sub = parts[1] if len(parts) > 1 else None
            if sub == "talk":
                field, kind = rec + 0xC, "sequence"
            elif sub == "defeat":
                act = self.w32(rec + 0x14)
                fn = self.w32(act)
                if fn not in (sa.ACTION_DIALOG, sa.ACTION_SCRIPT):
                    raise AsmError(f"{path}: the record has no defeat action")
                field, kind = act + 4, "sequence" if fn == sa.ACTION_DIALOG else "script"
            else:
                raise AsmError(f"{path}: use record:N/talk or record:N/defeat")
            target = self.w32(field)
        elif head in MAP_FIELDS:
            field, kind = me + MAP_FIELDS[head], "script"
            target = self.w32(field)
        else:
            raise AsmError(f"unknown edit path {path!r}")
        if entry is not None and kind != "sequence":
            raise AsmError(f"{path}: /entry:K needs a sequence")
        return field, target, kind, entry

    def edit_path(self, me, path, replace, tag):
        field, target, kind, entry = self.resolve(me, path)
        pairs = [(" ".join(a.split()), b) for a, b in replace]
        hits = {a: 0 for a, _ in pairs}

        def apply(lines):
            out = []
            for ln in lines:
                key = " ".join(ln.split())
                for a, b in pairs:
                    if key == a:
                        ln = ln[:len(ln) - len(ln.lstrip())] + b
                        hits[a] += 1
                        break
                out.append(ln)
            return out

        if kind == "script":
            lines = apply(sa.format_script_block(self.rom, target, "edit"))
            blk = sa.parse_file("\n".join(lines))["edit"]
            blk["addr"] = None
            new = self.put(sa.assemble_script(blk["items"]), 1, f"{tag} {path} (edited script)")
            self.write(field, struct.pack("<I", new), f"{tag} {path} -> {new:08x}")
        elif entry is None:
            lines = apply(sa.format_sequence(self.rom, target, "edit"))
            blk = sa.parse_file("\n".join(lines))["edit"]
            n = len(self.placer.records)
            new = sa.place_block(f"{tag} {path}", blk, self.stock, self.placer)
            self.sync(n)
            self.write(field, struct.pack("<I", new), f"{tag} {path} -> {new:08x}")
        else:
            ptrs, _ = sa.read_sequence(self.rom, target)
            if not 0 <= entry < len(ptrs):
                raise AsmError(f"{path}: the sequence has {len(ptrs)} entries")
            d = sa.decode_entry(self.rom, ptrs[entry])
            lines = apply(["sequence edit"] + sa.format_entry(d, entry) + ["end"])
            text = "\n".join(lines)
            # the index check in parse_entries counts from 0: renumber this lone entry
            text = text.replace(f"  [{entry}] ", "  [0] ", 1)
            e = sa.parse_file(text)["edit"]["entries"][0]
            e["next"] = d["next"] if "next" not in e["attrs"] else e["attrs"]["next"]
            n = len(self.placer.records)
            new = sa.place_entry(e, self.stock, self.placer, f"{tag} {path}")
            self.sync(n)
            self.write(target + 4 * entry, struct.pack("<I", new), f"{tag} {path} -> {new:08x}")
        missing = [a for a, c in hits.items() if not c]
        if missing:
            raise AsmError(f"{path}: no line matched {missing}")


def build(rom, specs, free=STORY_FREE, limit=STORY_LIMIT, editor=None):
    """Apply spec files (paths) in order; returns (records, editor)."""
    ed = editor or MapEditor(rom, free, limit)
    n = len(ed.records)
    for p in specs:
        ed.apply(json.load(open(p)), p)
    return ed.records[n:], ed


def main():
    args = sys.argv[1:]
    rom_path = "roms/log2.gba"
    free = STORY_FREE
    if "--rom" in args:
        i = args.index("--rom")
        rom_path = args[i + 1]
        del args[i:i + 2]
    if "--free" in args:
        i = args.index("--free")
        free = int(args[i + 1], 0)
        del args[i:i + 2]
    if len(args) < 2:
        sys.exit(__doc__)
    rom = open(rom_path, "rb").read()
    try:
        if args[0] == "build":
            recs, ed = build(rom, args[1:-1], free)
            json.dump(recs, open(args[-1], "w"), indent=1)
            print(f"{args[-1]}: {len(recs)} records, story data {free:#x} -> {ed.placer.free:#x}")
        elif args[0] == "show":
            recs, ed = build(rom, args[1:], free)
            for r in recs:
                print(f"  {r['offset']:>10} {len(r['bytes']) // 2:5} B  {r.get('note', '')}")
            print(f"{len(recs)} records, story data {free:#x} -> {ed.placer.free:#x}")
        else:
            sys.exit(__doc__)
    except AsmError as e:
        sys.exit(f"error: {e}")


if __name__ == "__main__":
    main()
