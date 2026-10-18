#!/bin/bash
# Fork instance slot: source this to get the ports and file names of instance FORK_SLOT (default 0).
# Slot 0 keeps the historical values (GDB 2345, pad 7778, /tmp/gdbin, /root/gdb.log, title "mGBA");
# slot n uses GDB 2345+n, pad 7778+n, /tmp/gdbin-n, /root/gdb-n.log, /root/mgba-n.log, title "mGBA-n",
# so several Claude sessions can each run their own fork. Pick a free slot with `ss -ltn`.
FORK_SLOT=${FORK_SLOT:-0}
GDB_PORT=$((2345 + FORK_SLOT))
PAD_PORT=$((7778 + FORK_SLOT))
if [ "$FORK_SLOT" = "0" ]; then SFX=""; TITLE="mGBA"; else SFX="-$FORK_SLOT"; TITLE="mGBA-$FORK_SLOT"; fi
GDB_FIFO=/tmp/gdbin$SFX
GDB_LOG=/root/gdb$SFX.log
GDB_PID=/root/gdb$SFX.pid
FORK_LOG=/root/mgba$SFX.log
