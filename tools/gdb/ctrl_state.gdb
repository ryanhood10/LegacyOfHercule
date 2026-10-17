# Control-state check for both players (mGBA GDB server). Offsets per build/coop.sym.
set pagination off
set confirm off
target remote 127.0.0.1:2345
set $p1 = *(unsigned int*)0x03001FA8
set $p2 = *(unsigned int*)0x0203ff08
printf "p1=%08x p2=%08x\n", $p1, $p2
if $p1
  printf "P1 ctrl=%08x vt=%08x pos=(%d,%d)\n", *(unsigned int*)($p1+0x1c), **(unsigned int**)($p1+0x1c), *(int*)($p1+0x158)>>8, *(int*)($p1+0x15c)>>8
end
if $p2
  printf "P2 ctrl=%08x vt=%08x pos=(%d,%d)\n", *(unsigned int*)($p2+0x1c), **(unsigned int**)($p2+0x1c), *(int*)($p2+0x158)>>8, *(int*)($p2+0x15c)>>8
end
printf "remote=%04x input2=%04x source_age=%d\n", *(unsigned short*)0x0203ff00, *(unsigned short*)0x0203ff7e, *(unsigned short*)0x0203ffba
disconnect
