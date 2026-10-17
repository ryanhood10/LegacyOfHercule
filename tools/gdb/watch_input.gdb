# Log every PC that reads the input block for a few frames, then quit.
# Run mGBA first:  mgba-sdl -g -t <savestate> roms\log2.gba
# Then:            arm-none-eabi-gdb -batch -x tools\gdb\watch_input.gdb
set pagination off
set confirm off
target remote :2345

# g_InputSystem.current / pressed / released
rwatch *(unsigned short*)0x030022AE
rwatch *(unsigned short*)0x030022B0
rwatch *(unsigned short*)0x030022B2

set $n = 0
while $n < 200
  continue
  printf "READ pc=%08x lr=%08x r0=%08x\n", $pc, $lr, $r0
  set $n = $n + 1
end
quit
