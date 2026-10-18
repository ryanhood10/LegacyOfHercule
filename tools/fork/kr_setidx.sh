#!/bin/bash
. /root/slot.sh
# Select ability INDEX (0-3 of its four-entry list) for party SLOT (PARTY_SLOTS builds; the party block
# is at 0x0203F000). usage: FORK_SLOT=n kr_setidx.sh SLOT INDEX
G=/root/gdbsrv.sh
$G pause; sleep 0.8
$G cmd "set *(unsigned char*)(0x0203F00C + $1*0x1c + 9) = $2"
sleep 0.4
$G cmd continue
sleep 0.4
