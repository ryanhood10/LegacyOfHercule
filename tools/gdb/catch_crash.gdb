# Attach to mGBA's GDB server (mGBA -g), let the game run, and dump the registers and stack when it
# stops (a crash such as "Jumped to invalid address" halts the core with a signal).
#   arm-none-eabi-gdb -batch -x tools\gdb\catch_crash.gdb > build\crash.txt
set pagination off
set confirm off
target remote 127.0.0.1:2345
continue
printf "stopped: pc=%08x lr=%08x sp=%08x\n", $pc, $lr, $sp
info registers
x/32xw $sp
x/8i $lr-16
detach
quit
