-- Wide-view renderer test: writes the view descriptor the custom mGBA reads (EWRAM 0x0203FEC0:
-- "WIDE", u16 width, u16 height) every frame, so the wide path can be checked before the ROM
-- side exists. Set wide_w / wide_h from the console; wide_w = 240 switches back to the stock view.
wide_w = tonumber(os.getenv("WIDE_W") or "480")
wide_h = tonumber(os.getenv("WIDE_H") or "320")
local frames = 0
callbacks:add("frame", function()
  frames = frames + 1
  if frames == 10 then
    local f = io.open("/tmp/wide_test.txt", "w")
    if f then f:write(string.format("frame callback alive, desc=%08x\n", emu:read32(0x0203FEC0))); f:close() end
  end
  emu:write32(0x0203FEC0, 0x45444957)
  emu:write16(0x0203FEC4, wide_w)
  emu:write16(0x0203FEC6, wide_h)
end)
console:log(string.format("wide_test: descriptor %dx%d", wide_w, wide_h))
local lf = io.open("/tmp/wide_load.txt", "w")
if lf then lf:write("script loaded; emu=" .. tostring(emu) .. " callbacks=" .. tostring(callbacks) .. "\n"); lf:close() end
