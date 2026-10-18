#!/bin/bash
# Warp both players to a map through the co-op test command (coop2.cmd = 5 -> the WarpToMapNoEntities
# opcode handler; src/coop/enemies.c): usage [FORK_SLOT=n] warp.sh ZONE AREA X Y
# e.g. warp.sh 15 1 216 200 (arctic), 15 2 312 280 (Time Chamber), 15 3 276 354 (Broly's arena).
# Pause it in game, not on the title or a menu: interrupting the stub while the CPU idles in the BIOS
# can drop the gdb connection.
. /root/slot.sh
G=/root/gdbsrv.sh
$G pause; sleep 0.5
$G cmd "set *(unsigned int*)0x0203FD0C = $1 | ($2 << 8)"
$G cmd "set *(unsigned int*)0x0203FD54 = $3 | ($4 << 16)"
$G cmd "set *(unsigned int*)0x0203FD08 = 5"
sleep 0.3
$G cmd continue
sleep 0.5
tail -2 "$GDB_LOG"
