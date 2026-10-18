# Android 16 fight readout for the fork's GDB stub (tools/fork/gdbsrv.sh: source /root/a16peek.gdb).
# Offsets: coop 0x0203FF04 (p2 +4, p2_slot +0x80, source_age +0xB6), coop2 0x0203FB00 (a16 +0x234,
# fired/booms +0x238, hits/skipped +0x23C, force +0x240, bot/specials +0x244, spawn/dead frame +0x248/+0x24C),
# party entries 0x0203F00C + slot * 0x1C (hp, max_hp, ep, max_ep, row, ability, str, pow, end, exp).
printf "frame=%d p1=%08x p2=%08x p2slot=%d age=%d active=%d\n", *(int*)0x03000E20, *(unsigned*)0x03001FA8, *(unsigned*)0x0203FF08, *(unsigned char*)0x0203FF84, *(unsigned short*)0x0203FFBA, *(unsigned char*)0x0203F000
printf "slot1 hp %d/%d str %d pow %d end %d exp %d | slot5 hp %d/%d str %d pow %d end %d exp %d\n", *(unsigned short*)0x0203F028, *(unsigned short*)0x0203F02A, *(unsigned short*)0x0203F032, *(unsigned short*)0x0203F034, *(unsigned short*)0x0203F036, *(unsigned*)0x0203F038, *(unsigned short*)0x0203F098, *(unsigned short*)0x0203F09A, *(unsigned short*)0x0203F0A2, *(unsigned short*)0x0203F0A4, *(unsigned short*)0x0203F0A6, *(unsigned*)0x0203F0A8
printf "a16=%08x fired=%d booms=%d hits=%d skipped=%d force=%d bot=%d specials=%d spawn=%d dead=%d\n", *(unsigned*)0x0203FD34, *(unsigned short*)0x0203FD38, *(unsigned short*)0x0203FD3A, *(unsigned short*)0x0203FD3C, *(unsigned short*)0x0203FD3E, *(unsigned*)0x0203FD40, *(unsigned short*)0x0203FD44, *(unsigned short*)0x0203FD46, *(unsigned*)0x0203FD48, *(unsigned*)0x0203FD4C
if *(unsigned*)0x0203FD34
  printf "a16 hp=%d x=%d y=%d facing=%d ctrl=%08x\n", *(int*)(*(unsigned*)0x0203FD34 + 0x188), *(int*)(*(unsigned*)0x0203FD34 + 0x158) >> 8, *(int*)(*(unsigned*)0x0203FD34 + 0x15C) >> 8, *(unsigned char*)(*(unsigned*)0x0203FD34 + 0x10), *(unsigned*)(*(unsigned*)0x0203FD34 + 0x18)
end
