#!/bin/bash
. "$(dirname "$0")/slot.sh"
# Hold player-1 buttons through the netplay pad word g_p1_remote (0x0203FF02, 0x8000 | active-high
# mask: A=1 B=2 Select=4 Start=8 Right=0x10 Left=0x20 Up=0x40 Down=0x80 R=0x100 L=0x200).
# usage: keys.sh MASK SECONDS [MASK SECONDS ...]   (a 0 mask = release / wait)
G="$(dirname "$0")/gdbsrv.sh"
while [ $# -ge 2 ]; do
  mask=$1; secs=$2; shift 2
  $G pause; sleep 0.4
  if [ "$mask" = "0" ]; then
    $G cmd 'set *(unsigned short*)0x0203FF02 = 0'
  else
    $G cmd "set *(unsigned short*)0x0203FF02 = 0x8000 | $mask"
  fi
  sleep 0.3
  $G cmd continue
  sleep "$secs"
done
$G pause; sleep 0.4
$G cmd 'set *(unsigned short*)0x0203FF02 = 0'
sleep 0.3
$G cmd continue
