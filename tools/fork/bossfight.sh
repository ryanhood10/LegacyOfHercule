#!/bin/bash
# Scripted boss-pair fight in a fork slot (an A16_TEST build, docs/testing.md "Boss pairs"): spawn enemy
# record ADDR beside player 1 (COOP_CMD_SPAWN_RECORD), turn on the test autopilot (both players walk
# to their boss and punch, step out of swings), print HP every STEP seconds until both bosses are down,
# a player's HP reaches 0, or SECS pass.
# usage: FORK_SLOT=n bossfight.sh ADDR [SECS=240] [STEP=5] [P1_ENTRY=0x0203F028] [P2_ENTRY=0x0203F098]
# P*_ENTRY = the players' party entries (hp u16 at +0); defaults = Piccolo (slot 1), Hercule (slot 5).
. "$(dirname "$0")/slot.sh"
G="$(dirname "$0")/gdbsrv.sh"
C=0x0203FB00
ADDR=$1; SECS=${2:-240}; STEP=${3:-5}; E1=${4:-0x0203F028}; E2=${5:-0x0203F098}
$G pause; sleep 0.6
$G cmd "set *(unsigned*)($C+0x20C) = $ADDR"
$G cmd "set *(unsigned*)($C+0x208) = 8"
$G cmd "set *(unsigned short*)($C+0x244) = 1"
sleep 0.3; $G cmd continue
t=0
while [ "$t" -le "$SECS" ]; do
  sleep "$STEP"; t=$((t + STEP))
  $G pause; sleep 0.5
  $G cmd "set \$o = *(unsigned*)($C+0x124)"
  $G cmd "set \$w = *(unsigned*)($C+0x128)"
  $G cmd "printf \"FIGHT$$ t=$t orig=%d twin=%d p1=%d p2=%d downed=%d\\n\", \$o ? *(int*)(\$o+0x188) : -1, \$w ? *(int*)(\$w+0x188) : -1, *(unsigned short*)$E1, *(unsigned short*)$E2, *(unsigned short*)($C+0x250)"
  sleep 0.5; $G cmd continue; sleep 0.2
  line=$(grep "FIGHT$$ t=$t " "$GDB_LOG" | tail -1 | sed 's/^(gdb) //')
  echo "$line"
  o=$(echo "$line" | sed -n 's/.*orig=\(-\?[0-9]*\).*/\1/p'); w=$(echo "$line" | sed -n 's/.*twin=\(-\?[0-9]*\).*/\1/p')
  p1=$(echo "$line" | sed -n 's/.* p1=\([0-9]*\).*/\1/p'); p2=$(echo "$line" | sed -n 's/.* p2=\([0-9]*\).*/\1/p')
  [ -z "$o" ] && continue
  if [ "$o" -le 0 ] && [ "$w" -le 0 ]; then echo "RESULT win t=$t p1=$p1 p2=$p2"; break; fi
  if [ "$p1" = 0 ] || [ "$p2" = 0 ]; then echo "RESULT ko t=$t orig=$o twin=$w p1=$p1 p2=$p2"; break; fi
done
$G pause; sleep 0.5
$G cmd "set *(unsigned short*)($C+0x244) = 0"
sleep 0.3; $G cmd continue
