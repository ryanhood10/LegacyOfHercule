-- Console override for player 2's pad (testing without a second keyboard/sender).
-- dofile(...) after p2_input.lua, then:  p2_force = 0x8001  (A held)   p2_force = 0x8000 (nothing)
-- p2_force = nil hands control back to the TCP bridge. Bits 0-9 = GBA keys active-high, bit 15 = valid.
p2_force = nil
callbacks:add("frame", function()
  if p2_force then emu:write16(0x0203FF00, p2_force) end
end)
console:log("force loaded: set p2_force = 0x8000 | keys, nil to release")
