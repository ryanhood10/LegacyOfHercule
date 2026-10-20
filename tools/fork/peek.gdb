# PARTY_SLOTS 9 layout (the default since 2026-10-19): tail offsets = stock + 3 * 0x1C, save slots of 0x1C0
printf "frame=%d player=%08x old@03000E90=%08x %08x\n", *(int*)0x03000E20, *(unsigned int*)0x03001FA8, *(unsigned int*)0x03000E90, *(unsigned int*)0x03000E94
printf "party: active=%d flags=%02x zone/area/var=%d/%d/%d meter=%d pos=%d,%d\n", *(unsigned char*)0x0203F000, *(unsigned char*)0x0203F001, *(unsigned char*)0x0203F002, *(unsigned char*)0x0203F003, *(unsigned char*)0x0203F004, *(int*)0x0203F008, *(short*)0x0203F1B4, *(short*)0x0203F1B6
set $i = 0
while $i < 9
  set $c = 0x0203F00C + $i * 0x1C
  printf "slot %d: hp %d/%d ep %d/%d row %d str %d exp %d flags %08x ab %d %d %d %d\n", $i, *(unsigned short*)$c, *(unsigned short*)($c+2), *(unsigned short*)($c+4), *(unsigned short*)($c+6), *(unsigned char*)($c+8), *(unsigned short*)($c+0xA), *(unsigned int*)($c+0x10), *(unsigned int*)($c+0x14), *(unsigned char*)($c+0x18), *(unsigned char*)($c+0x19), *(unsigned char*)($c+0x1A), *(unsigned char*)($c+0x1B)
  set $i = $i + 1
end
printf "flags+108: %02x %02x %02x %02x  seen+16E: %02x %02x  counters+17F: %02x %02x %02x\n", *(unsigned char*)0x0203F108, *(unsigned char*)0x0203F109, *(unsigned char*)0x0203F10A, *(unsigned char*)0x0203F10B, *(unsigned char*)0x0203F16E, *(unsigned char*)0x0203F16F, *(unsigned char*)0x0203F17F, *(unsigned char*)0x0203F180, *(unsigned char*)0x0203F181
if *(unsigned int*)0x03001FA8
  printf "p1 pos=%d,%d\n", *(int*)(*(unsigned int*)0x03001FA8+0x158)>>8, *(int*)(*(unsigned int*)0x03001FA8+0x15C)>>8
end
set $m = *(unsigned int*)0x03003898
printf "eeprom mirror %08x slot0: %08x %08x %08x  slot1: %08x  slot2: %08x\n", $m, *(unsigned int*)($m+0x10), *(unsigned int*)($m+0x14), *(unsigned int*)($m+0x18), *(unsigned int*)($m+0x10+0x1C0), *(unsigned int*)($m+0x10+0x380)
printf "mirror tail @0203EFF8: %08x %08x | party head: %08x %08x\n", *(unsigned int*)0x0203EFF8, *(unsigned int*)0x0203EFFC, *(unsigned int*)0x0203F000, *(unsigned int*)0x0203F004
printf "--end--\n"
