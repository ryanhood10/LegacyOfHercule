#!/bin/bash
# Pause the fork, print the Android 16 fight readout (a16peek.gdb), continue. Extra args = gdb commands
# run while paused (e.g. 'set *(unsigned*)0x0203FD40 = 3' to force bombs).
G=/root/gdbsrv.sh
$G pause; sleep 0.4
for c in "$@"; do $G cmd "$c"; sleep 0.2; done
$G cmd 'source /root/a16peek.gdb'; sleep 0.8
$G cmd continue; sleep 0.3
grep -E "^(\(gdb\) )*(frame=|slot1|a16)" /root/gdb.log | tail -4
