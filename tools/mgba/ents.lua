-- Entity list dump for the mGBA Scripting console: dofile(...) then call ents()
-- Prints the camera view origin, both players and every enemy (vtable 0x080252A4) in g_EntityList
-- (0x03001120): index, vtable, position, behaviour word 0 (base EXP for enemies), control state.
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

function ents(all)
  local list = 0x03001120
  local count = emu:read32(list + 4)
  local p1 = emu:read32(0x03001FA8)
  local p2 = emu:read32(COOP + 4)
  console:log(string.format("entities=%d events=%d view=(%d,%d) p1=%s p2=%s", count, emu:read32(list + 0x610),
    emu:read32(0x030010AC), emu:read32(0x030010B0), hex(p1), hex(p2)))
  for i = 0, count - 1 do
    local e = emu:read32(list + 8 + i * 4)
    local vt = emu:read32(e)
    local tag = (e == p1 and "P1") or (e == p2 and "P2") or (vt == 0x080252A4 and "EN") or nil
    if tag or all then
      local beh = emu:read32(e + 0x180)
      local w0 = beh ~= 0 and emu:read32(beh) or 0
      console:log(string.format("%2d %s vt=%s pos=(%d,%d) w0=%d s18=%s", i, tag or "  ", hex(vt),
        emu:read32(e + 0x158) >> 8, emu:read32(e + 0x15C) >> 8, w0, hex(emu:read32(e + 0x18))))
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
