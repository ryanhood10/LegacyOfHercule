#!/bin/bash
. "$(dirname "$0")/slot.sh"
# pause the fork, run an optional extra gdb command, then /root/peek.gdb; continue; print the dump
G="$(dirname "$0")/gdbsrv.sh"
$G pause; sleep 0.7
$G cmd 'printf "--begin--\n"'
[ -n "$1" ] && $G cmd "$1"
$G cmd "source $(dirname "$0")/peek.gdb"
sleep 1.5
$G cmd continue
sleep 0.3
sed -n '/--begin--/,/--end--/p' "$GDB_LOG" | tail -24 | sed 's/^(gdb) //'
