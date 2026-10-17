# Boot smoke test for a patched ROM: wait for the game loop to run a few hundred frames, then report.
# Run mGBA first:  mgba-sdl -g build\log2-coop.gba
# Then:            arm-none-eabi-gdb -batch -x tools\gdb\check_boot.gdb
set pagination off
set confirm off
target remote :2345

# GameLoop_Tick's call to Input_Poll: hit once per frame
break *0x0801F2B6
set $n = 0
while $n < 300
  continue
  set $n = $n + 1
end
printf "frame counter  = %d\n", *(int*)0x03000E20
printf "idle slot 2    = %08x (expect 007da5bd)\n", *(unsigned int*)0x08025A8C
printf "code @8800040  = %08x (expect 4f23b5f0)\n", *(unsigned int*)0x08800040
printf "coop.magic     = %08x\n", *(unsigned int*)0x0203FF00
printf "coop.p2        = %08x\n", *(unsigned int*)0x0203FF04
printf "g_PlayerObject = %08x\n", *(unsigned int*)0x03001FA8
kill
quit
