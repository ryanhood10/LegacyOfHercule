-- mGBA script: receive player 2's buttons over TCP and hand them to the co-op ROM every frame.
-- Load from Tools > Scripting in mGBA (Qt). Sender: tools/p2_keys.py (same PC or remote).
-- Wire format: 2 bytes little-endian, bits 0-9 = GBA keys active-high (A B Sel Start R L U D R L).

local PORT = 7777
local REMOTE_ADDR = nil   -- filled in from build/coop.sym (g_p2_remote) below, or set by hand
local VALID = 0x8000

-- Try to read the address from the symbol file next to the ROM, else fall back to g_p2_remote's
-- pinned address 0x0203FF00 (docs/ids.md "Memory"; tools/mgba/netplay.lua uses the same).
local function readSymbol()
  local f = io.open("coop.sym", "r") or io.open("F:/MeltyCreations/log2/build/coop.sym", "r")
  if not f then return nil end
  for line in f:lines() do
    local addr = line:match("^(%x+) %a g_p2_remote")
    if addr then f:close(); return tonumber(addr, 16) end
  end
  f:close()
  return nil
end
REMOTE_ADDR = REMOTE_ADDR or readSymbol() or 0x0203FF00

local latest = 0
local connected = false
local server = socket.bind("0.0.0.0", PORT)
local client = nil

local function drain()
  -- read everything queued; the newest complete word wins
  local guard = 0
  while client and client:hasdata() and guard < 64 do
    local data = client:receive(256)
    if not data or #data == 0 then break end
    if #data >= 2 then
      local b1, b2 = data:byte(#data - 1, #data)
      latest = b1 + b2 * 256
    end
    guard = guard + 1
  end
end

local function onData() drain() end

local function onError()
  client = nil
  connected = false
  latest = 0
  console:log("p2_input: sender disconnected")
end

local function onAccept()
  local c = server:accept()
  if not c then return end
  if client then client:close() end
  client = c
  connected = true
  client:add("received", onData)
  client:add("error", onError)
  console:log("p2_input: sender connected")
end

server:add("received", onAccept)
server:listen(1)
console:log(string.format("p2_input: listening on %d, writing to %08X", PORT, REMOTE_ADDR))

callbacks:add("frame", function()
  server:poll()
  if client then client:poll(); drain() end
  if emu then
    if connected then emu:write16(REMOTE_ADDR, (latest & 0x3FF) | VALID)
    else emu:write16(REMOTE_ADDR, 0) end
  end
end)
