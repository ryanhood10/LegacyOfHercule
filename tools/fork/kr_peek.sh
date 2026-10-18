#!/bin/bash
. /root/slot.sh
# Party readout (PARTY_SLOTS 8 builds): active slot, every entry (row, hp, ep, stats, exp, flags,
# abilities), player 2's slot and entity, player 1's entity; extra gdb commands as arguments; continue.
# usage: FORK_SLOT=n kr_peek.sh ['gdb command' ...]
G=/root/gdbsrv.sh
$G pause; sleep 1
$G cmd 'printf "--begin--\n"'
$G cmd 'printf "active %d\n", *(unsigned char*)0x0203F000'
for s in 0 1 2 3 4 5 6 7; do
  a=$((0x0203F00C + s*0x1C))
  $G cmd "printf \"slot $s row %d hp %d/%d ep %d/%d str %d pow %d end %d exp %d flags %x ab %d %d %d %d sel %d\n\", *(unsigned char*)$((a+8)), *(unsigned short*)$a, *(unsigned short*)$((a+2)), *(short*)$((a+4)), *(unsigned short*)$((a+6)), *(unsigned short*)$((a+10)), *(unsigned short*)$((a+12)), *(unsigned short*)$((a+14)), *(unsigned int*)$((a+16)), *(unsigned int*)$((a+20)), *(unsigned char*)$((a+24)), *(unsigned char*)$((a+25)), *(unsigned char*)$((a+26)), *(unsigned char*)$((a+27)), *(unsigned char*)$((a+9))"
done
$G cmd 'printf "p2 %x p2slot %d p1 %x\n", *(unsigned int*)0x0203FF08, *(unsigned char*)0x0203FF84, *(unsigned int*)0x03001FA8'
for c in "$@"; do $G cmd "$c"; done
$G cmd 'printf "--end--\n"'
sleep 2
$G cmd continue
sleep 0.5
tac "$GDB_LOG" | sed '/--begin--/q' | tac | sed 's/^(gdb) //'
