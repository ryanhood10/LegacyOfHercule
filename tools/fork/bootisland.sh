#!/bin/bash
# From a fresh boot of a DEBUG_MENU=1 build in fork slot FORK_SLOT: wait for the debug menu, Map Test
# zone 11 area 6 (the island path, no triggers), nudge player 1 so player 2 joins, then give both players
# level-30 stats (Piccolo slot 1, Hercule slot 5; docs/testing.md "Android 16 boss fight") and place
# them on open ground (player 1 left, player 2 right of where COOP_CMD_SPAWN_RECORD puts a boss pair). Needs a player-2 sender (tools/fork/p2hold.py --port 7778+slot) running.
# usage: FORK_SLOT=n bootisland.sh [MENU_WAIT=105]
. "$(dirname "$0")/slot.sh"
G="$(dirname "$0")/gdbsrv.sh"
sleep "${1:-105}"
S="0x80 0.3 0 0.3 0x80 0.3 0 0.3"
for i in $(seq 10); do S="$S 0x10 0.3 0 0.3"; done
S="$S 0x80 0.3 0 0.3"
for i in $(seq 5); do S="$S 0x10 0.3 0 0.3"; done
S="$S 1 0.3 0 5 0x10 0.4 0 2"
"$(dirname "$0")/keys.sh" $S
$G pause; sleep 0.6
for c in \
  'set *(unsigned short*)0x0203F028 = 554' 'set *(unsigned short*)0x0203F02A = 554' \
  'set *(unsigned short*)0x0203F032 = 11955' 'set *(unsigned short*)0x0203F034 = 12750' 'set *(unsigned short*)0x0203F036 = 10140' \
  'set *(unsigned short*)0x0203F098 = 245' 'set *(unsigned short*)0x0203F09A = 245' \
  'set *(unsigned short*)0x0203F0A2 = 12030' 'set *(unsigned short*)0x0203F0A4 = 11650' 'set *(unsigned short*)0x0203F0A6 = 16130' \
  'set *(unsigned char*)(*(unsigned*)0x03001FA8+0x160) = 46' 'set *(unsigned char*)(*(unsigned*)0x03001FA8+0x161) = 49' 'set *(unsigned char*)(*(unsigned*)0x03001FA8+0x162) = 39' \
  'set *(unsigned char*)(*(unsigned*)0x0203FF08+0x160) = 47' 'set *(unsigned char*)(*(unsigned*)0x0203FF08+0x161) = 45' 'set *(unsigned char*)(*(unsigned*)0x0203FF08+0x162) = 63' \
  'set *(int*)(*(unsigned*)0x03001FA8+0x158) = 89<<8' 'set *(int*)(*(unsigned*)0x03001FA8+0x15C) = 347<<8' \
  'set *(int*)(*(unsigned*)0x0203FF08+0x158) = 200<<8' 'set *(int*)(*(unsigned*)0x0203FF08+0x15C) = 330<<8' \
  'printf "BOOT p2=%x frame=%d\n", *(unsigned*)0x0203FF08, *(unsigned*)0x03000E20'; do
  $G cmd "$c"
done
sleep 0.8; $G cmd continue; sleep 0.5
grep -a "BOOT" "$GDB_LOG" | tail -1
