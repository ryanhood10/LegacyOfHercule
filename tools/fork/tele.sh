#!/bin/bash
. /root/slot.sh
# teleport player 1 (pixels). usage: tele.sh X Y
G=/root/gdbsrv.sh
$G pause; sleep 0.5
$G cmd "set *(int*)(*(unsigned int*)0x03001FA8+0x158) = $1 << 8"
$G cmd "set *(int*)(*(unsigned int*)0x03001FA8+0x15C) = $2 << 8"
sleep 0.3
$G cmd continue
