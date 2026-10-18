#!/bin/bash
# One gdb query against a running fork slot: pause, run COMMAND (it prints one line starting "Q:"),
# wait for that line, continue, print it. gdb's output reaches the log late (an earlier query's line can
# arrive after a later command), so each call tags its "Q:" with its own id and waits for that tag.
# usage: FORK_SLOT=n q.sh 'printf "Q: hp=%d\n", *(int*)0x...'
. "$(dirname "$0")/slot.sh"
G="$(dirname "$0")/gdbsrv.sh"
tag="Q$$_$RANDOM:"
$G pause; sleep 0.2
$G cmd "${1//Q:/$tag}"
for i in $(seq 50); do grep -aq "$tag" "$GDB_LOG" && break; sleep 0.1; done
$G cmd continue; sleep 0.2
grep -a "$tag" "$GDB_LOG" | tail -1 | sed "s/^(gdb) //; s/$tag/Q:/"
