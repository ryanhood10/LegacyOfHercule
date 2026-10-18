#!/bin/bash
. /root/slot.sh
# Run an Android 16 fight on the running fork (started from a savestate by tools/fork/runa16.ps1):
# spawn him (COOP_CMD_A16), optionally force one special and turn on the autopilot, log every hit a
# player takes (a16hits.gdb), print the readout every STEP seconds for up to SECS seconds or until his
# HP or a player's party HP reaches 0. usage: a16fight.sh FORCE BOT SECS [STEP]
FORCE=${1:-0}; BOT=${2:-0}; SECS=${3:-60}; STEP=${4:-5}
/root/a16peek.sh 'source /root/a16hits.gdb' "set *(unsigned*)0x0203FD40 = $FORCE" "set *(unsigned short*)0x0203FD44 = $BOT" 'set *(unsigned*)0x0203FD08 = 4' > /dev/null
t=0
while [ $t -lt $SECS ]; do
  sleep $STEP; t=$((t + STEP))
  out=$(/root/a16peek.sh)
  echo "== t=$t"; echo "$out" | tail -4
  echo "$out" | grep -q "a16 hp=0 \|a16 hp=-" && { echo "BOSS DOWN"; break; }
  echo "$out" | grep -q "slot1 hp 0/\|slot5 hp 0/" && { echo "PLAYER DOWN"; break; }
done
echo "== hits"; grep -oE "(HIT|BOSS) .*" "$GDB_LOG" > /root/a16hits.log; wc -l /root/a16hits.log
