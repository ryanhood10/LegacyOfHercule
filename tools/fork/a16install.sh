#!/bin/bash
# Copy the Android 16 fork helpers to /root (CR stripped): a16peek.sh/.gdb, a16hits.gdb, a16fight.sh.
D=/mnt/f/MeltyCreations/log2-a16/tools/fork
for f in a16peek.sh a16peek.gdb a16hits.gdb a16fight.sh; do tr -d '\r' < "$D/$f" > "/root/$f"; done
chmod +x /root/a16peek.sh /root/a16fight.sh
echo installed
