-- One-stop script for the co-op ROM: loads the player-2 TCP bridge plus the console helpers
-- (peek(), ents(), p2_force). Use from mGBA's Scripting window or on the command line:
--   mGBA.exe --script F:/MeltyCreations/log2/tools/mgba/coop.lua build/log2-coop.gba
local dir = (debug.getinfo(1, "S").source:match("^@(.*[/\\])") or "./")
dofile(dir .. "p2_input.lua")
dofile(dir .. "peek.lua")
dofile(dir .. "ents.lua")
dofile(dir .. "force.lua")
dofile(dir .. "enemies.lua")
console:log("coop.lua: bridge + peek() + ents() + p2_force + spd()/track()/hits() ready")
