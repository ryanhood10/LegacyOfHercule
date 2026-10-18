#!/bin/bash
# Paused-frame screenshot series of an Android 16 fight on the fork (Git Bash on Windows):
# a16shots.sh PREFIX COUNT RUN_SECONDS [FORCE BOT]  -> build/a16/shots/PREFIX<n>.png
# The emulator is paused for each screenshot, so RUN_SECONDS of game time pass between shots.
PRE=$1; N=$2; RUN=$3; FORCE=$4; BOT=$5
W="wsl -d Ubuntu -u root -- bash -c"
if [ -n "$FORCE" ]; then
  MSYS_NO_PATHCONV=1 $W "/root/a16peek.sh 'set *(unsigned*)0x0203FD40 = $FORCE' 'set *(unsigned short*)0x0203FD44 = $BOT' 'set *(unsigned*)0x0203FD08 = 4'" > /dev/null
fi
for i in $(seq 1 "$N"); do
  sleep "$RUN"
  MSYS_NO_PATHCONV=1 $W "/root/gdbsrv.sh pause"; sleep 0.5
  powershell -NoProfile -Command "& F:\MeltyCreations\log2-a16\tools\win_shot.ps1 F:\MeltyCreations\log2-a16\build\a16\shots\$PRE$i.png -Title 'mGBA (Ubuntu)'" > /dev/null
  MSYS_NO_PATHCONV=1 $W "/root/gdbsrv.sh cmd continue"
done
