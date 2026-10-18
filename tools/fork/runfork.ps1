# Launch the WSL mGBA fork on a ROM with the GDB stub, attach the persistent gdb client, let it run.
# usage: runfork.ps1 <rom path under /mnt/f/...> [-Wait seconds] [-Slot n] [-State /mnt/f/.../x.ssN] [-Quiet]
# Slot n (default 0) = GDB port 2345+n, pad port 7778+n, window title "mGBA-n" (slot 0: "mGBA"),
# gdb log /root/gdb-n.log; only that slot's instance is stopped first, so sessions can run in parallel.
# Installs tools/fork/*.sh and *.gdb into /root (CR stripped) on every launch.
param([string]$Rom, [int]$Wait = 14, [int]$Slot = 0, [string]$State = "", [switch]$Quiet)
$gdbPort = 2345 + $Slot
$padPort = 7778 + $Slot
$title = if ($Slot -eq 0) { "mGBA" } else { "mGBA-$Slot" }
$sfx = if ($Slot -eq 0) { "" } else { "-$Slot" }
$st = if ($State) { "-t $State" } else { "" }
# -Quiet: no sound and no audio sync; with several forks running, audio sync starves an instance (it
# barely advances and stops draining its pad socket), docs/testing.md
$q = if ($Quiet) { "-C mute=1 -C audioSync=0" } else { "" }
wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < /mnt/f/MeltyCreations/log2/tools/fork/install.sh | bash > /dev/null; FORK_SLOT=$Slot /root/gdbsrv.sh stop; fuser -k $gdbPort/tcp $padPort/tcp 2>/dev/null; echo stopped slot $Slot"
Start-Sleep -Milliseconds 500
Start-Process -FilePath wsl -ArgumentList @('-d', 'Ubuntu', '-u', 'root', '--', 'bash', '-c', "trap '' PIPE; cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=$padPort MGBA_GDB_PORT=$gdbPort MGBA_TITLE=$title exec ./mgba -2 $q -g $st $Rom > /root/mgba$sfx.log 2>&1") -WindowStyle Hidden
Start-Sleep -Seconds 3
$n = 0; while (-not (wsl -d Ubuntu -u root -- bash -c "ss -ltn | grep -q ':$gdbPort ' && echo y") -and $n -lt 30) { Start-Sleep -Seconds 1; $n++ }
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=$Slot /root/gdbsrv.sh start > /dev/null; FORK_SLOT=$Slot /root/waitlog.sh 20 'Remote debugging' 1 > /dev/null; FORK_SLOT=$Slot /root/gdbsrv.sh cmd continue; echo running slot $Slot gdb $gdbPort pad $padPort title $title"
Start-Sleep -Seconds $Wait
