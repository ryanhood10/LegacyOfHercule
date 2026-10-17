-- Focus-free test harness: a TCP line server inside mGBA (port 7779). Each line received is run as
-- Lua in the frame callback; everything the helpers console:log() during it comes back, then "<END>".
-- Client: python tools/mgba_cmd.py 'ents()'   (or any Lua)
-- Extras for driving the game without the keyboard (the co-op ROM reads both pads from EWRAM):
--   seq("DOWN:5 WAIT:20 A:3")   queue player-1 keys in frames (g_p1_remote, bit 15 = valid); names
--                               A B SELECT START RIGHT LEFT UP DOWN R L, "A+RIGHT:10" holds several;
--                               "RELEASE" hands the pad back to the keyboard at the end
--   p2seq("...")                same for player 2 (sets p2_force each frame; "RELEASE" sets it nil)
--   busy()                      frames left in both queues
--   state(n) / save(n)          load / save savestate slot n
--   shot("path.png")            screenshot
--   flush()                     return the lines frame callbacks logged since the last command
local PORT = 7779
local P1_WORD, P2_WORD = 0x0203FF02, 0x0203FF00
local KEYS = { A = 1, B = 2, SELECT = 4, START = 8, RIGHT = 0x10, LEFT = 0x20, UP = 0x40, DOWN = 0x80, R = 0x100, L = 0x200 }

-- capture console output: lines logged while a command runs go back with it; lines logged by frame
-- callbacks in between (hits(), spdwatch()) are kept in a backlog that flush() returns.
local real_console = cmd_real_console or console
cmd_real_console = real_console
local buffer = nil
local backlog = {}
local function capture(s)
  if buffer then buffer[#buffer + 1] = s else backlog[#backlog + 1] = s end
end
console = {
  log = function(_, s) real_console:log(s); capture(tostring(s)) end,
  error = function(_, s) real_console:error(s); capture("ERROR " .. tostring(s)) end,
}
function flush()
  for _, s in ipairs(backlog) do buffer[#buffer + 1] = s end
  backlog = {}
end

local function parse(script)
  local q = {}
  for step in script:gmatch("%S+") do
    local name, n = step:match("^(.-):(%d+)$")
    if not name then name, n = step, 1 end
    local mask = 0
    if name == "WAIT" then mask = 0
    elseif name == "RELEASE" then mask = -1
    else
      for k in name:gmatch("[^+]+") do mask = mask | (KEYS[k:upper()] or error("unknown key " .. k)) end
    end
    q[#q + 1] = { mask = mask, frames = tonumber(n) }
  end
  return q
end

local q1, q2 = {}, {}
function seq(s) for _, st in ipairs(parse(s)) do q1[#q1 + 1] = st end end
function p2seq(s) for _, st in ipairs(parse(s)) do q2[#q2 + 1] = st end end
function busy()
  local a, b = 0, 0
  for _, st in ipairs(q1) do a = a + st.frames end
  for _, st in ipairs(q2) do b = b + st.frames end
  return a + b
end
function state(n) emu:loadStateSlot(n) ; console:log("state " .. n .. " loaded") end
function save(n) emu:saveStateSlot(n) ; console:log("state " .. n .. " saved") end
function shot(path) emu:screenshot(path); console:log("shot " .. path) end

local function drive(q, write)
  local st = q[1]
  if not st then return end
  if st.mask == -1 then write(nil); table.remove(q, 1); return end
  write(0x8000 | st.mask)
  st.frames = st.frames - 1
  if st.frames <= 0 then
    table.remove(q, 1)
    if #q == 0 then write(0x8000) end
  end
end

-- reload-safe: drop the previous server's socket and frame callback
if cmd_server_socket then cmd_server_socket:close() end
if cmd_server_cb then callbacks:remove(cmd_server_cb) end
local server = socket.bind("127.0.0.1", PORT)
server:listen(2)
cmd_server_socket = server
local clients = {}
local pending = {}

local function run_line(line)
  buffer = {}
  local fn, err = load(line, "cmd", "t")
  if not fn then buffer[#buffer + 1] = "ERROR " .. tostring(err)
  else
    local ok, res = pcall(fn)
    if not ok then buffer[#buffer + 1] = "ERROR " .. tostring(res)
    elseif res ~= nil then buffer[#buffer + 1] = tostring(res) end
  end
  local out = table.concat(buffer, "\n")
  buffer = nil
  return out
end

cmd_server_cb = callbacks:add("frame", function()
  drive(q1, function(v) emu:write16(P1_WORD, v or 0) end)
  drive(q2, function(v) p2_force = v end)
  -- accept
  local c = server:accept()
  while c do clients[#clients + 1] = c; pending[c] = ""; c = server:accept() end
  for i = #clients, 1, -1 do
    local cl = clients[i]
    local alive = true
    while cl:hasdata() do
      local data = cl:receive(1024)
      if not data or #data == 0 then alive = false; break end
      pending[cl] = pending[cl] .. data
    end
    local line, rest = pending[cl]:match("^(.-)\n(.*)$")
    while line do
      pending[cl] = rest
      local out = run_line(line)
      local msg = out .. "\n<END>\n"
      local sent = 0
      while sent < #msg do
        local n = cl:send(msg:sub(sent + 1))
        if not n then alive = false; break end
        sent = sent + n
      end
      line, rest = pending[cl]:match("^(.-)\n(.*)$")
    end
    if not alive then pending[cl] = nil; table.remove(clients, i) end
  end
end)
console:log("cmd_server: listening on " .. PORT .. " (seq, p2seq, state, save, shot)")
