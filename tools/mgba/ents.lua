-- Entity list dump for the mGBA Scripting console: dofile(...) then call ents()
-- Prints the camera view origin, both players and every enemy (vtable 0x080252A4, or the co-op copy
-- in coop2) in g_EntityList (0x03001120): index, tag (EN enemy, CE co-op enemy, TW twin), vtable,
-- position, HP / max HP (stat +4), base EXP (stat +0), control state.
local function hex(v) return string.format("%08x", v) end
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
local COOP2 = symbol("coop2", 0x0203FB00)
local ENEMY_VT = 0x080252A4
local COOP_ENEMY_VT = COOP2 + 4          -- CoopExtra.enemy_vtable
local TWINS = COOP2 + 4 + 48 * 4 + 24 * 4 -- CoopExtra.twins[24] {orig, twin}

local function is_twin(e)
  for i = 0, 23 do
    if emu:read32(TWINS + i * 8 + 4) == e then return true end
  end
  return false
end

function ents(all)
  local list = 0x03001120
  local count = emu:read32(list + 4)
  local p1 = emu:read32(0x03001FA8)
  local p2 = emu:read32(COOP + 4)
  local entry = emu:read32(0x03002084)   -- map state +4: the MapEntry in use (+4 = script count)
  console:log(string.format("entities=%d events=%d view=(%d,%d) p1=%s p2=%s twins=%d scaled=%d bars=%d map Z%dA%d entry=%s scripts=%d", count,
    emu:read32(list + 0x610), emu:read32(0x030010AC), emu:read32(0x030010B0), hex(p1), hex(p2),
    emu:read16(TWINS + 24 * 8), emu:read16(TWINS + 24 * 8 + 2), emu:read16(TWINS + 24 * 8 + 4),
    emu:read8(0x03002080), emu:read8(0x03002081), hex(entry), entry ~= 0 and emu:read8(entry + 4) or 0))
  for i = 0, count - 1 do
    local e = emu:read32(list + 8 + i * 4)
    local vt = emu:read32(e)
    local enemy = vt == ENEMY_VT or vt == COOP_ENEMY_VT
    local tag = (e == p1 and "P1") or (e == p2 and "P2") or (vt == ENEMY_VT and "EN")
      or (vt == COOP_ENEMY_VT and (is_twin(e) and "TW" or "CE")) or nil
    if tag or all then
      local beh = emu:read32(e + 0x180)
      local w0 = beh ~= 0 and emu:read32(beh) or 0
      local hp = enemy and string.format(" hp=%d/%d spd=%d", emu:read32(e + 0x188), emu:read32(beh + 4), emu:read8(e + 0x160)) or ""
      console:log(string.format("%2d %s @%s vt=%s pos=(%d,%d) w0=%d%s s18=%s", i, tag or "  ", hex(e), hex(vt),
        emu:read32(e + 0x158) >> 8, emu:read32(e + 0x15C) >> 8, w0, hp, hex(emu:read32(e + 0x18))))
    end
  end
end
console:log("ents loaded")

-- words(addr, n): log n 32-bit words starting at addr
function words(addr, n)
  local t = {}
  for i = 0, (n or 8) - 1 do t[#t + 1] = string.format("%x=%08x", addr + i * 4, emu:read32(addr + i * 4)) end
  console:log(table.concat(t, " "))
end
