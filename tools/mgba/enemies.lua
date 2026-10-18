-- Enemy-scaling checks for the mGBA Scripting console (loaded by coop.lua).
--   spd()        lists every live chase-speed AI module in EWRAM ({vtable, speed 8.8}); co-op enemies'
--                modules carry the scaled speed (e.g. 0xB3 -> 0xD6 for the wolf chase)
--   spdwatch()   scans for chase modules every 20 frames and logs new ones; spdwatch(false) stops
--   track(e, n)  logs entity e's displacement over the next n frames (default 60)
--   hits()       logs the players' HP every time it changes (damage per hit); hits(false) stops
--   twins()      lists the twin registry: each original / twin with its position and nearest player
local SPEED8_VT, SPEED4_VT = 0x08023BE0, 0x08023D88

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
local PARTY = symbol("g_party_block", 0x03000E90)   -- relocated party block
local TWINS = COOP2 + 4 + 48 * 4 + 24 * 4
local DBG = TWINS + 24 * 8   -- spawned, scaled, bars, dmg_calls, off, pad, unused (u16), dmg_player, dmg_scaled, last_dmg...

-- dmg(): the damage-hook debug counters and the last calculation
function dmg()
  console:log(string.format("dmg calls=%d player=%d scaled=%d off=%d last dmg=%d secondary=%d defender=%08x attacker=%08x",
    emu:read16(DBG + 6), emu:read16(DBG + 14), emu:read16(DBG + 16), emu:read16(DBG + 8), emu:read32(DBG + 20), emu:read32(DBG + 24),
    emu:read32(DBG + 28), emu:read32(DBG + 32)))
end
-- spawnrec(i): spawn mapScripts[i] of the current map the way the TriggerMapEvent opcode does (boss records
-- with condition word 1 only ever spawn that way); bar(e): draw the boss bar(s) for entity e (0 = pair 0)
local CMD = DBG + 36
function spawnrec(i) emu:write32(CMD + 4, i); emu:write32(CMD, 1); console:log("spawn script " .. i) end
function bar(e) emu:write32(CMD + 4, e or 0); emu:write32(CMD, 2); console:log("boss bar requested") end
-- actor(sprite): stand a script actor of that sprite id beside player 1 (to see what a sprite id looks like)
function actor(sprite) emu:write32(CMD + 4, sprite); emu:write32(CMD, 3); console:log("actor sprite " .. sprite) end
-- lowhp(n): set every registered original and twin to n HP (pair-rule tests)
function lowhp(n)
  for i = 0, 23 do
    for _, off in ipairs({0, 4}) do
      local e = emu:read32(TWINS + i * 8 + off)
      if e ~= 0 then emu:write32(e + 0x188, n or 1) end
    end
  end
  console:log("registered enemies set to " .. (n or 1) .. " HP")
end
-- tele(dx): move every registered original and twin beside player 1 (dx px right, default 24)
function tele(dx)
  local p = emu:read32(0x03001FA8)
  if p == 0 then return end
  for i = 0, 23 do
    for _, off in ipairs({0, 4}) do
      local e = emu:read32(TWINS + i * 8 + off)
      if e ~= 0 then
        emu:write32(e + 0x158, emu:read32(p + 0x158) + ((dx or 24) << 8))
        emu:write32(e + 0x15C, emu:read32(p + 0x15C))
      end
    end
  end
  console:log("registered enemies moved beside player 1")
end
-- partner(sprite): make every boss twin use this sprite id (0 = the built-in partner table)
function partner(sprite) emu:write32(CMD + 8 + 28 + 4, sprite or 0); console:log("partner override " .. (sprite or 0)) end
-- coopoff(bits): tuning switches, bit 1 = damage multiplier, 2 = twins, 4 = speed, 8 = HP scaling, 16 = partner models (0 = all on)
function coopoff(bits) emu:write16(DBG + 8, bits or 0); console:log("coop2.off = " .. (bits or 0)) end

local function scan_modules(seen)
  local found = {}
  for a = 0x02000800, 0x0203FBFC, 4 do
    local v = emu:read32(a)
    if v == SPEED8_VT or v == SPEED4_VT then
      if not (seen and seen[a] == emu:read32(a + 4)) then
        found[#found + 1] = string.format("%08x %s speed=%d (%.2f px/frame)", a, v == SPEED8_VT and "chase8" or "chase4",
          emu:read32(a + 4), emu:read32(a + 4) / 256)
      end
      if seen then seen[a] = emu:read32(a + 4) end
    end
  end
  return found
end

function spd()
  local found = scan_modules(nil)
  for _, line in ipairs(found) do console:log(line) end
  console:log("speed modules: " .. #found)
end

local watch = nil
function spdwatch(on)
  watch = (on ~= false) and { seen = {}, n = 0 } or nil
  console:log("speed watch " .. (watch and "on" or "off"))
end

local trk = nil
function track(e, n)
  trk = { e = e, x = emu:read32(e + 0x158), y = emu:read32(e + 0x15C), n = 0, limit = n or 60 }
end

local hp_watch = false
local last = {}
function hits(on)
  hp_watch = on ~= false
  last = {}
  console:log("hp watch " .. (hp_watch and "on" or "off"))
end

local function dist2(e, p)
  local dx = (emu:read32(e + 0x158) - emu:read32(p + 0x158)) >> 8
  local dy = (emu:read32(e + 0x15C) - emu:read32(p + 0x15C)) >> 8
  return dx * dx + dy * dy
end

function twins()
  local p1 = emu:read32(0x03001FA8)
  local p2 = emu:read32(COOP + 4)
  local function near(e)
    if e == 0 then return "-" end
    if p1 == 0 or p2 == 0 then return "?" end
    return dist2(e, p2) < dist2(e, p1) and "P2" or "P1"
  end
  local function pos(e)
    if e == 0 then return "dead" end
    return string.format("(%d,%d) hp=%d near=%s", emu:read32(e + 0x158) >> 8, emu:read32(e + 0x15C) >> 8,
      emu:read32(e + 0x188), near(e))
  end
  for i = 0, 23 do
    local o, t = emu:read32(TWINS + i * 8), emu:read32(TWINS + i * 8 + 4)
    if o ~= 0 or t ~= 0 then
      console:log(string.format("pair %2d orig %08x %s | twin %08x %s", i, o, pos(o), t, pos(t)))
    end
  end
end

callbacks:add("frame", function()
  if trk then
    trk.n = trk.n + 1
    if trk.n >= trk.limit then
      console:log(string.format("track %08x: dx=%d dy=%d over %d frames", trk.e,
        (emu:read32(trk.e + 0x158) - trk.x) >> 8, (emu:read32(trk.e + 0x15C) - trk.y) >> 8, trk.n))
      trk = nil
    end
  end
  if hp_watch then
    for slot = 0, 5 do
      local hp = emu:read16(PARTY + 0xC + slot * 0x1C)
      if last[slot] and last[slot] ~= hp then
        console:log(string.format("frame %d slot %d hp %d -> %d (%+d)", emu:read32(0x03000E20), slot, last[slot], hp, hp - last[slot]))
      end
      last[slot] = hp
    end
  end
  if watch then
    watch.n = watch.n + 1
    if watch.n % 20 == 0 then
      for _, line in ipairs(scan_modules(watch.seen)) do console:log("new " .. line) end
    end
  end
end)
console:log("enemies helpers loaded: spd(), spdwatch(), track(e, n), hits(), twins()")
