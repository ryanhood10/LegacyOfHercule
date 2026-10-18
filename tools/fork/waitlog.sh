#!/bin/bash
# wait up to $1 seconds for a pattern $2 in /root/gdb.log, then print the tail
for i in $(seq 1 "$1"); do grep -q -E "$2" /root/gdb.log && break; sleep 1; done
tail -n "${3:-6}" /root/gdb.log
pgrep -a gdb-multiarch | head -1
