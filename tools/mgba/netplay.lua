-- mGBA script: frame-lockstep netplay for the co-op ROM (mGBA 0.10.x Qt, Tools > Scripting).
-- Both players run build/log2-coop.gba, connect to tools/netplay_relay.py, and exchange one pad word
-- per frame; neither emulation runs a frame before it has both pads for it, so the two stay identical.
--
-- Console, host (player 1 on mGBA's own keys):
--   NETPLAY = { role = "host", relay = "127.0.0.1", port = 7778, delay = 2 }
--   dofile("F:/MeltyCreations/log2/tools/mgba/netplay.lua")
-- Console, guest (player 2 on its mGBA keys):
--   NETPLAY = { role = "guest", relay = "<relay ip>", port = 7778 }
--   dofile("F:/MeltyCreations/log2/tools/mgba/netplay.lua")
--
-- Handshake: both send HELLO with the ROM CRC; the host then snapshots its emulation (savestate, with
-- save data) and sends it, the guest loads it, and lockstep frame 1 starts on both from that snapshot.
-- So the host decides the starting point: reset first for a fresh boot, or load a savestate first.
-- Inputs sampled at the end of frame N apply at frame N+delay on both sides (delay 2 = one frame of
-- network slack: no stall while one-way latency < 16 ms; use 3-4 over the internet).
-- Every hash_every frames both sides hash a few game words (player positions, co-op state) and compare;
-- every deep_every frames the whole IWRAM; mismatches are logged, and the host re-sends its state
-- (resync = true, default). Pad words: bits 0-9 GBA keys active-high, bit 15 valid, written to
-- g_p1_remote (0x0203FF02) and g_p2_remote (0x0203FF00) on both machines (addresses from coop.sym).
-- Console helpers afterwards: netplay_status(), netplay_resync() (host), netplay_stop().

local cfg = NETPLAY or {}
local ROLE = cfg.role
if ROLE ~= "host" and ROLE ~= "guest" then
  error('set NETPLAY = { role = "host" or "guest", relay = "ip", port = 7778, delay = 2 } before dofile')
end
local RELAY = cfg.relay or "127.0.0.1"
local PORT = cfg.port or 7778
local DELAY = math.max(1, cfg.delay or 2)
local HASH_EVERY = cfg.hash_every or 60
local DEEP_EVERY = cfg.deep_every or 300
local WAIT_MS = cfg.timeout or 10000        -- how long to wait for the peer's input before giving up
local RESYNC = cfg.resync ~= false
local QUIET = cfg.quiet or 600              -- frames between "in sync" log lines
local PROTO = 1

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
local P2_ADDR = symbol("g_p2_remote", 0x0203FF00)
local P1_ADDR = symbol("g_p1_remote", 0x0203FF02)
local COOP = symbol("coop", 0x0203FF04)
local COOP_SIZE = 0xDC
local P1_ENTITY = 0x03001FA8      -- g_PlayerObject
local FRAME_COUNTER = 0x03000E20  -- g_FrameCounter
local SAVE_FLAGS = (C and C.SAVESTATE and (C.SAVESTATE.SAVEDATA | C.SAVESTATE.RTC)) or 10

local function log(fmt, ...) console:log("netplay: " .. string.format(fmt, ...)) end
local function warn(fmt, ...) console:warn("netplay: " .. string.format(fmt, ...)) end
local function err(fmt, ...) console:error("netplay: " .. string.format(fmt, ...)) end

local sock, buf = nil, ""
local phase = "off"          -- off | hello | sync | run
local frame = 0              -- lockstep frame index (0 = the snapshot both sides started from)
local mine, theirs = {}, {}  -- frame -> pad word
local myHash, theirHash = {}, {}
local pendingState = nil     -- bytes of STATE payload still expected
local peerDelay = nil
local stats = { stalls = 0, waited = 0, checks = 0, desyncs = 0, resyncs = 0 }
local crc

local function romCrc()
  local ok, s = pcall(function() return emu:checksum(C.CHECKSUM.CRC32) end)
  if ok and s and #s == 4 then
    return string.format("%02x%02x%02x%02x", s:byte(1, 4))
  end
  return string.format("size%d", emu:romSize())
end

local function dropPads()
  emu:write16(P1_ADDR, 0)
  emu:write16(P2_ADDR, 0)
end

local function drop(reason)
  if sock then sock:close() end
  sock, buf, phase, pendingState = nil, "", "off", nil
  dropPads()
  err("stopped: %s (frame %d)", reason, frame)
end

local function sendall(data)
  local i, spins = 1, 0
  while sock and i <= #data do
    local n, e = sock:send(data, i)
    if n then
      if n <= 0 then spins = spins + 1 else i = i + n end
    elseif e == "temporary failure" then
      spins = spins + 1
    else
      drop("send failed: " .. tostring(e)); return false
    end
    if spins > 200000 then drop("send stalled"); return false end
  end
  return sock ~= nil
end

local function pump()  -- move everything readable into buf; false once the peer is gone
  while sock do
    local ready, e = sock:hasdata()
    if e then drop("socket error: " .. tostring(e)); return false end
    if not ready then return true end
    local data, e2 = sock:receive(65536)
    if not data then drop(e2 == "disconnected" and "peer disconnected" or ("receive failed: " .. tostring(e2))); return false end
    buf = buf .. data
  end
  return false
end

local function block(ms)  -- wait until the socket is readable or ms elapsed; true if readable
  if not sock then return false end
  local raw = rawget(sock, "_s")
  if raw and raw.select then return raw:select(ms) > 0 end
  return true
end

local function nextMessage()
  if pendingState then
    if #buf < pendingState then return nil end
    local data = buf:sub(1, pendingState)
    buf = buf:sub(pendingState + 1)
    pendingState = nil
    return "STATEDATA", data
  end
  local nl = buf:find("\n", 1, true)
  if not nl then return nil end
  local line = buf:sub(1, nl - 1)
  buf = buf:sub(nl + 1)
  return line
end

-- Hash of the words that matter most, or (deep) of the whole IWRAM too.
local function hashState(deep)
  local h = 2166136261
  local function mix(v)
    for i = 0, 3 do h = ((h ~ ((v >> (i * 8)) & 0xFF)) * 16777619) & 0xFFFFFFFF end
  end
  local function entity(ptrAddr)
    local e = emu:read32(ptrAddr)
    mix(e)
    if e >= 0x02000000 and e < 0x04000000 then
      mix(emu:read32(e + 0x158)); mix(emu:read32(e + 0x15C))   -- position (8.8 fixed)
      mix(emu:read32(e + 0x18)); mix(emu:read32(e + 0x10))     -- control state, facing
    end
  end
  entity(P1_ENTITY)
  entity(COOP + 4)
  for off = 0, COOP_SIZE - 4, 4 do mix(emu:read32(COOP + off)) end
  mix(emu:read32(FRAME_COUNTER))
  if deep then
    local ok, s = pcall(function() return emu:readRange(0x03000000, 0x8000) end)
    if ok and s then
      for i = 1, #s do h = ((h ~ s:byte(i)) * 16777619) & 0xFFFFFFFF end
    end
  end
  return h
end

local go, resync, handle

local function compareHash(f)
  local a, b = myHash[f], theirHash[f]
  if a == nil or b == nil then return end
  stats.checks = stats.checks + 1
  if a == b then
    if f % QUIET == 0 then
      log("in sync at frame %d (hash %08x, stalls %d, checks %d)", f, a, stats.stalls, stats.checks)
    end
  else
    stats.desyncs = stats.desyncs + 1
    err("DESYNC at frame %d: local %08x remote %08x", f, a, b)
    if RESYNC and ROLE == "host" and phase == "run" then resync() end
  end
  myHash[f], theirHash[f] = nil, nil
  for k in pairs(theirHash) do if k < f - 2 * DEEP_EVERY then theirHash[k] = nil end end
  for k in pairs(myHash) do if k < f - 2 * DEEP_EVERY then myHash[k] = nil end end
end

local function startSync()   -- host: snapshot and ship it, then wait for the guest to load it
  local state = emu:saveStateBuffer(SAVE_FLAGS)
  if not state or #state == 0 then drop("saveStateBuffer failed"); return end
  phase = "sync"
  log("sending %d-byte snapshot", #state)
  if not sendall(string.format("STATE %d\n", #state)) then return end
  if not sendall(state) then return end
  local waited = 0
  while sock and phase == "sync" do
    if not pump() then return end
    local line, data = nextMessage()
    while line do
      handle(line, data)
      line, data = nextMessage()
    end
    if phase == "sync" then
      if not block(50) then waited = waited + 50 end
      if waited > 60000 then drop("guest never confirmed the snapshot"); return end
    end
  end
end

function resync()
  stats.resyncs = stats.resyncs + 1
  log("resync %d", stats.resyncs)
  if not sendall("RESYNC\n") then return end
  startSync()
end

function go()  -- both: the current frame boundary is lockstep frame 0
  frame = 0
  mine, theirs, myHash, theirHash = {}, {}, {}, {}
  for f = 1, DELAY - 1 do mine[f] = 0; theirs[f] = 0 end
  phase = "run"
  log("lockstep running as %s, delay %d frame(s)", ROLE, DELAY)
end

function handle(line, data)
  local cmd, rest = line:match("^(%S+)%s*(.*)$")
  if cmd == "I" then
    local f, w = rest:match("^(%d+) (%d+)$")
    f, w = tonumber(f), tonumber(w)
    if not f then warn("bad input line %q", line); return end
    if phase ~= "run" then return end                 -- stale, from before a resync
    if f < frame then warn("input for frame %d arrived at frame %d", f, frame); return end
    if theirs[f] ~= nil and theirs[f] ~= w then warn("duplicate input for frame %d differs", f) end
    theirs[f] = w
  elseif cmd == "H" then
    local f, h = rest:match("^(%d+) (%x+)$")
    if not f then warn("bad hash line %q", line); return end
    if phase ~= "run" then return end
    theirHash[tonumber(f)] = tonumber(h, 16)
    compareHash(tonumber(f))
  elseif cmd == "HELLO" then
    local proto, role, theirCrc, delay = rest:match("^(%d+) (%a+) (%w+) (%d+)$")
    if not proto then drop("bad HELLO " .. line); return end
    if tonumber(proto) ~= PROTO then drop("protocol " .. proto .. " vs ours " .. PROTO); return end
    if role == ROLE then drop("both sides are " .. ROLE); return end
    if theirCrc ~= crc then drop("ROM mismatch: peer " .. theirCrc .. ", ours " .. crc); return end
    peerDelay = tonumber(delay)
    if ROLE == "guest" and peerDelay ~= DELAY then
      log("using the host's delay %d", peerDelay); DELAY = peerDelay
    elseif ROLE == "host" and peerDelay ~= DELAY then
      log("guest asked for delay %d, host keeps %d", peerDelay, DELAY)
    end
    log("peer connected (%s, ROM %s)", role, theirCrc)
    if ROLE == "host" then startSync() end
  elseif cmd == "RESYNC" then
    if ROLE == "guest" then phase = "sync"; log("host is resyncing") end
  elseif cmd == "STATE" then
    pendingState = tonumber(rest)
    phase = "sync"
    log("receiving %d-byte snapshot", pendingState or -1)
  elseif cmd == "STATEDATA" then
    if ROLE ~= "guest" then return end
    local ok, res = pcall(function() return emu:loadStateBuffer(data, SAVE_FLAGS) end)
    if not ok or res == false then drop("loadStateBuffer failed: " .. tostring(res)); return end
    if sendall("READY\n") then go() end
  elseif cmd == "READY" then
    if ROLE == "host" and phase == "sync" then go() end
  elseif cmd == "BYE" then
    drop("peer left")
  else
    warn("unknown message %q", line)
  end
end

local function apply(f)
  local p1 = ROLE == "host" and mine[f] or theirs[f]
  local p2 = ROLE == "host" and theirs[f] or mine[f]
  emu:write16(P1_ADDR, 0x8000 | (p1 & 0x3FF))
  emu:write16(P2_ADDR, 0x8000 | (p2 & 0x3FF))
  mine[f], theirs[f] = nil, nil
end

local function step()  -- end of lockstep frame `frame`; make frame+1 possible
  local word = emu:getKeys() & 0x3FF
  local target = frame + DELAY
  mine[target] = word
  if not sendall(string.format("I %d %d\n", target, word)) then return end
  frame = frame + 1
  local waited, stalled = 0, false
  while sock and phase == "run" and theirs[frame] == nil do
    if not pump() then return end
    local line, data = nextMessage()
    while line do
      handle(line, data)
      line, data = nextMessage()
    end
    if phase ~= "run" then return end
    if theirs[frame] == nil then
      stalled = true
      if not block(20) then
        waited = waited + 20
        if waited > WAIT_MS then drop(string.format("no input from the peer for %d ms", waited)); return end
      end
    end
  end
  if not sock or phase ~= "run" then return end
  if stalled then stats.stalls = stats.stalls + 1; stats.waited = stats.waited + waited end
  apply(frame)
  if frame % HASH_EVERY == 0 then
    local deep = DEEP_EVERY > 0 and frame % DEEP_EVERY == 0
    local h = hashState(deep)
    myHash[frame] = h
    if sendall(string.format("H %d %08x\n", frame, h)) then compareHash(frame) end
  end
end

callbacks:add("frame", function()
  if not sock then return end
  if phase == "run" then
    step()
  else
    if not pump() then return end
    local line, data = nextMessage()
    while line and sock do
      handle(line, data)
      if phase == "run" then step(); break end   -- the snapshot just became frame 0
      line, data = nextMessage()
    end
  end
end)

function netplay_status()
  log("role %s phase %s frame %d delay %d stalls %d (%d ms) checks %d desyncs %d resyncs %d",
    ROLE, phase, frame, DELAY, stats.stalls, stats.waited, stats.checks, stats.desyncs, stats.resyncs)
end

function netplay_resync()
  if ROLE ~= "host" or phase ~= "run" then warn("resync is for a running host"); return end
  resync()
end

function netplay_stop()
  if sock then sendall("BYE\n") end
  drop("stopped from the console")
end

crc = romCrc()
local s, e = socket.connect(RELAY, PORT)
if not s then error(string.format("netplay: cannot reach relay %s:%d (%s)", RELAY, PORT, tostring(e))) end
sock = s
phase = "hello"
dropPads()
sendall(string.format("HELLO %d %s %s %d\n", PROTO, ROLE, crc, DELAY))
log("%s connected to relay %s:%d, ROM %s; waiting for the peer", ROLE, RELAY, PORT, crc)
