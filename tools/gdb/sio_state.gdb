# Dump the serial registers and co-op state of a running game via mGBA's GDB server (Tools > Start GDB server).
# arm-none-eabi-gdb -batch -x tools\gdb\sio_state.gdb      (offsets follow build/coop.sym: g_p2_remote, coop)
set pagination off
set confirm off
target remote 127.0.0.1:2345
printf "pc        = %08x\n", $pc
printf "SIOCNT    = %04x\n", *(unsigned short*)0x04000128
printf "RCNT      = %04x\n", *(unsigned short*)0x04000134
printf "SIOMULTI0 = %04x  SIOMULTI1 = %04x\n", *(unsigned short*)0x04000120, *(unsigned short*)0x04000122
printf "MLT_SEND  = %04x\n", *(unsigned short*)0x0400012a
printf "g_p2_remote = %04x\n", *(unsigned short*)0x0203ff00
printf "coop.magic= %08x  coop.p2 = %08x\n", *(unsigned int*)0x0203ff04, *(unsigned int*)0x0203ff08
printf "input2 cur/pressed = %04x %04x\n", *(unsigned short*)0x0203ff5e, *(unsigned short*)0x0203ff60
printf "polls     = %d  sio_ready = %d  sio_last = %04x\n", *(unsigned short*)0x0203ff94, *(unsigned char*)0x0203ff96, *(unsigned short*)0x0203ff98
disconnect
