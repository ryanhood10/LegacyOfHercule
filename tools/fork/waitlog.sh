#!/bin/bash
# wait up to $1 seconds for a pattern $2 in the slot's gdb log, then print the tail ($3 lines)
. /root/slot.sh
for i in $(seq 1 "$1"); do grep -q -E "$2" "$GDB_LOG" && break; sleep 1; done
tail -n "${3:-6}" "$GDB_LOG"
