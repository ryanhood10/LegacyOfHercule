-- Console override for player 2's pad (testing without a second keyboard/sender).
-- dofile(...) after p2_input.lua, then:  p2_force = 0x8001  (A held)   p2_force = 0x8000 (nothing)
-- p2_force = nil hands control back to the TCP bridge. Bits 0-9 = GBA keys active-high, bit 15 = valid.
p2_force = nil
local seq = nil   -- { {word, frames}, ... } played by p2_play, one entry after another
function p2_play(s) seq = s end
-- double-tap dash in a direction (GBA key bits): tap, release, hold
function p2_dash(keys, hold) p2_play({ {0x8000 | keys, 4}, {0x8000, 6}, {0x8000 | keys, hold or 90}, {0x8000, 1} }) end
callbacks:add("frame", function()
  if seq then
    local step = seq[1]
    if not step then seq = nil; p2_force = 0x8000; return end
    emu:write16(0x0203FF00, step[1])
    step[2] = step[2] - 1
    if step[2] <= 0 then table.remove(seq, 1) end
    return
  end
  if p2_force then emu:write16(0x0203FF00, p2_force) end
end)
console:log("force loaded: set p2_force = 0x8000 | keys, nil to release")
