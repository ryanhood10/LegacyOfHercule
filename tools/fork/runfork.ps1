# Launch the WSL mGBA fork on a ROM with the GDB stub, attach the persistent gdb client, let it run.
# usage: runfork.ps1 <rom path under /mnt/f/...> [-Wait seconds] [-Slot n] [-State /mnt/f/.../x.ssN] [-Mute | -Quiet | -NoAudio]
# -Mute / -Quiet / -NoAudio (same thing) run without audio (-C mute=1 -C audioSync=0): with several forks
# running, audio sync can stall or starve an instance (stuck boot, ~1 fps, undrained pad socket); muted it
# runs at full speed.
# Slot n (default 0) = GDB port 2345+n, pad port 7778+n, window title "mGBA-n" (slot 0: "mGBA"),
# gdb log /root/gdb-n.log; only that slot's instance is stopped first, so sessions can run in parallel.
# Installs this checkout's tools/fork/*.sh and *.gdb (CR stripped) on every launch: slot 0 into /root,
# slot n into /root/fork-sn (run that slot's helpers from there, e.g. FORK_SLOT=3 /root/fork-s3/keys.sh).
# Slots >= 1 run the emulator as mgba-sN and gdb as gdb-sN (symlinks), out of reach of old helpers'
# `pkill -x mgba` / `pkill -INT -x gdb-multiarch` (a stray SIGINT disconnects gdb for the rest of the run).
param([string]$Rom, [int]$Wait = 14, [int]$Slot = 0, [string]$State = "", [switch]$Mute, [switch]$Quiet, [switch]$NoAudio)
$gdbPort = 2345 + $Slot
$padPort = 7778 + $Slot
$title = if ($Slot -eq 0) { "mGBA" } else { "mGBA-$Slot" }
$sfx = if ($Slot -eq 0) { "" } else { "-$Slot" }
$exe = if ($Slot -eq 0) { "mgba" } else { "mgba-s$Slot" }
$st = if ($State) { "-t $State" } else { "" }
$q = if ($Mute -or $Quiet -or $NoAudio) { "-C mute=1 -C audioSync=0" } else { "" }
$dest = if ($Slot -eq 0) { "/root" } else { "/root/fork-s$Slot" }   # this slot's helper copies
$tools = "/mnt/" + $PSScriptRoot.Substring(0, 1).ToLower() + $PSScriptRoot.Substring(2).Replace('\', '/')
wsl -d Ubuntu -u root -- bash -c "tr -d '\r' < $tools/install.sh | FORK_TOOLS=$tools FORK_DEST=$dest bash > /dev/null; ln -sf mgba /root/mgba-build/sdl/mgba-s$Slot; FORK_SLOT=$Slot $dest/gdbsrv.sh stop; fuser -k $gdbPort/tcp $padPort/tcp 2>/dev/null; echo stopped slot $Slot"
Start-Sleep -Milliseconds 500
Start-Process -FilePath wsl -ArgumentList @('-d', 'Ubuntu', '-u', 'root', '--', 'bash', '-c', "trap '' PIPE; cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=$padPort MGBA_GDB_PORT=$gdbPort MGBA_TITLE=$title exec ./$exe -2 $q -g $st $Rom > /root/mgba$sfx.log 2>&1") -WindowStyle Hidden
Start-Sleep -Seconds 3
$n = 0; while (-not (wsl -d Ubuntu -u root -- bash -c "ss -ltn | grep -q ':$gdbPort ' && echo y") -and $n -lt 30) { Start-Sleep -Seconds 1; $n++ }
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=$Slot $dest/gdbsrv.sh start > /dev/null; FORK_SLOT=$Slot $dest/waitlog.sh 20 'Remote debugging' 1 > /dev/null; FORK_SLOT=$Slot $dest/gdbsrv.sh cmd continue; echo running slot $Slot gdb $gdbPort pad $padPort title $title"
Start-Sleep -Seconds $Wait
