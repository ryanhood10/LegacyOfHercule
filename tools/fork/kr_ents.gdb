# list the scene's entities: index, address, vtable (0x08025604 player 1, 0x0203FE00 player 2,
# 0x080252A4 / the coop2 copy enemies), position (px), sprite record. usage: source /root/kr_ents.gdb
set $n = *(int*)0x03001124
set $i = 0
while $i < $n
  set $e = *(int*)(0x03001128 + $i*4)
  printf "%d %x vt %x xy %d %d spr %x\n", $i, $e, *(int*)$e, *(int*)($e+0x158)>>8, *(int*)($e+0x15c)>>8, *(int*)($e+0x48)
  set $i = $i + 1
end
