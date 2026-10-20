# Dump co-op runtime state via mGBA's GDB server (Tools > Start GDB server), offsets per build/coop.sym.
# Party: the relocated block g_party_block at 0x0203F000 (docs/ids.md "Memory"), PARTY_SLOTS entries
# (default 9; set $slots below for other builds). The stock block at 0x03000E90 is unused.
set pagination off
set confirm off
target remote 127.0.0.1:2345
set $p1 = *(unsigned int*)0x03001FA8
set $p2 = *(unsigned int*)0x0203ff08
printf "g_PlayerObject = %08x  coop.p2 = %08x  p2_slot = %d  active_slot = %d\n", $p1, $p2, *(unsigned char*)0x0203ff84, *(unsigned char*)0x0203F000
if $p1
  printf "P1 pos  = (%d, %d) px  facing %d  ctrl_state %08x\n", *(int*)($p1+0x158) >> 8, *(int*)($p1+0x15c) >> 8, *(unsigned char*)($p1+0x10), *(unsigned int*)($p1+0x1c)
end
if $p2
  printf "P2 pos  = (%d, %d) px  facing %d  ctrl_state %08x\n", *(int*)($p2+0x158) >> 8, *(int*)($p2+0x15c) >> 8, *(unsigned char*)($p2+0x10), *(unsigned int*)($p2+0x1c)
end
set $slots = 9
set $i = 0
while $i < $slots
  set $c = 0x0203F000 + 0xC + $i * 0x1C
  printf "slot %d: hp %d/%d ep %d/%d row %d flags %08x\n", $i, *(unsigned short*)$c, *(unsigned short*)($c+2), *(unsigned short*)($c+4), *(unsigned short*)($c+6), *(unsigned char*)($c+8), *(unsigned int*)($c+0x14)
  set $i = $i + 1
end
printf "g_p2_remote = %04x  input2 cur/pressed = %04x %04x  input1 cur = %04x\n", *(unsigned short*)0x0203ff00, *(unsigned short*)0x0203ff7e, *(unsigned short*)0x0203ff80, *(unsigned short*)0x030022AE
disconnect
