-- Wide-view renderer test: writes the view descriptor the custom mGBA reads (EWRAM 0x0203FEC0:
-- "WIDE", u16 width, u16 height) every frame, so the wide path can be checked before the ROM
-- side exists. Set wide_w / wide_h from the console; wide_w = 240 switches back to the stock view.
wide_w = tonumber(os.getenv("WIDE_W") or "480")
wide_h = tonumber(os.getenv("WIDE_H") or "320")
callbacks:add("frame", function()
  emu:write32(0x0203FEC0, 0x45444957)
  emu:write16(0x0203FEC4, wide_w)
  emu:write16(0x0203FEC6, wide_h)
end)
console:log(string.format("wide_test: descriptor %dx%d", wide_w, wide_h))
