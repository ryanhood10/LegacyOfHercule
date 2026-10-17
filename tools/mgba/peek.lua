-- Live co-op state readout for the mGBA Scripting console: dofile(...) then call peek()
-- Offsets follow build/coop.sym (g_p2_remote 0x0203FF00, coop 0x0203FF04).
local function hex(v) return string.format("%08x", v) end

-- base of the CoopState struct from the symbol file (falls back to the pinned address)
local function symbol(name, fallback)
  local f = io.open("coop.sym", "r") or io.open("F:/MeltyCreations/log2/build/coop.sym", "r")
  if not f then return fallback end
  for line in f:lines() do
    local addr = line:match("^(%x+) %a " .. name .. "$")
    if addr then f:close(); return tonumber(addr, 16) end
  end
  f:close()
  return fallback
end
local COOP = symbol("coop", 0x0203FF04)

function peek()
  local p1 = emu:read32(0x03001FA8)
  local p2 = emu:read32(COOP + 4)
  local function ent(name, e)
    if e == 0 then console:log(name .. ": none"); return end
    local s18 = emu:read32(e + 0x18)
    local s1c = emu:read32(e + 0x1C)
    local v18 = s18 ~= 0 and emu:read32(s18) or 0
    local v1c = s1c ~= 0 and emu:read32(s1c) or 0
    console:log(string.format("%s @%s pos=(%d,%d) +18=%s vt=%s  +1C=%s vt=%s", name, hex(e),
      emu:read32(e + 0x158) >> 8, emu:read32(e + 0x15C) >> 8, hex(s18), hex(v18), hex(s1c), hex(v1c)))
  end
  ent("P1", p1)
  ent("P2", p2)
  console:log(string.format("coop@%s remote=%04x input2=%04x age=%d slot2=%d active=%d spawns=%d destroys=%d", hex(COOP),
    emu:read16(0x0203FF00), emu:read16(COOP + 0x7A), emu:read16(COOP + 0xB6), emu:read8(COOP + 0x80), emu:read8(0x03000E90),
    emu:read16(COOP + 0xB8), emu:read16(COOP + 0xBA)))
  local function exp(slot) return emu:read32(0x03000E90 + 0xC + slot * 0x1C + 0x10) end
  local function hp(slot) return emu:read16(0x03000E90 + 0xC + slot * 0x1C) end
  local a, b = emu:read8(0x03000E90), emu:read8(COOP + 0x80)
  console:log(string.format("exp: active=%d p2slot=%d hp=%d/%d acting=%s awards p1=%d p2=%d rebinds=%d", exp(a), exp(b), hp(a), hp(b),
    hex(emu:read32(COOP + 0xBC)), emu:read16(COOP + 0xD8), emu:read16(COOP + 0xDA), emu:read16(COOP + 0xDE)))
  if p1 ~= 0 and p2 ~= 0 then
    console:log(string.format("sprites: p1=%s p2=%s", hex(emu:read32(p1 + 0x48)), hex(emu:read32(p2 + 0x48))))
  end
end
console:log("peek loaded")
