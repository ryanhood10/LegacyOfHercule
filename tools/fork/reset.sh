#!/bin/bash
. "$(dirname "$0")/slot.sh"
# jump the CPU to the ROM entry (ARM mode): a reset that keeps the emulator's EEPROM
G="$(dirname "$0")/gdbsrv.sh"
$G pause; sleep 0.5
$G cmd 'printf "reset from pc=%08x cpsr=%08x\n", $pc, $cpsr'
$G cmd 'set $cpsr = $cpsr & ~0x20'
$G cmd 'set $pc = 0x08000000'
sleep 0.3
$G cmd continue
sleep 0.5
grep 'reset from' "$GDB_LOG" | tail -1
