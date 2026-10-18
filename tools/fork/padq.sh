#!/bin/bash
# Queue MASK:SECONDS steps for a running tools/fork/p2hold.py (started with --cmd FILE) and wait until they
# have played. usage (Git Bash / WSL): padq.sh FILE "0x10:0.3 0:0.5 ..."
# Masks are the fork pad's GBA keys (A=1 B=2 Select=4 Start=8 Right=0x10 Left=0x20 Up=0x40 Down=0x80
# R=0x100 L=0x200). In KR_TEST builds 0x30C (L+R+Select+Start) toggles the pad between player 2 and player 1.
f=$1; shift
while [ -e "$f" ]; do sleep 0.1; done
echo "$*" > "$f"
while [ -e "$f" ]; do sleep 0.1; done
t=0; for st in $*; do t=$(awk -v a="$t" -v b="${st#*:}" 'BEGIN{print a+b}'); done
sleep "$t"
