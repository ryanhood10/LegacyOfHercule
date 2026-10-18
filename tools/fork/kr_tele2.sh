#!/bin/bash
. /root/slot.sh
# Teleport both players (vtables 0x08025604 / 0x0203FE00) to X Y px; the shared-screen leash otherwise pulls
# one player back toward the other. usage: FORK_SLOT=n kr_tele2.sh X Y
G=/root/gdbsrv.sh
$G pause; sleep 0.8
$G cmd 'set $n = *(int*)0x03001124'
$G cmd 'set $i = 0'
$G cmd "while \$i < \$n
set \$e = *(int*)(0x03001128 + \$i*4)
if *(int*)\$e == 0x08025604 || *(int*)\$e == 0x0203fe00
set *(int*)(\$e+0x158) = $1 << 8
set *(int*)(\$e+0x15c) = $2 << 8
end
set \$i = \$i + 1
end"
sleep 0.5
$G cmd continue
