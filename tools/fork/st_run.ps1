# Launch the WSL fork without the GDB stub for STORY_TEST builds (the stub has dropped connections and
# frozen the emulator; these builds are driven through the TCP pad by st_pad.py instead).
# usage: st_run.ps1 <rom under /mnt/f/...> [-Slot 1] [-State /mnt/f/.../x.ssN] [-X 0 -Y 0]
param([string]$Rom, [int]$Slot = 1, [string]$State = "", [int]$X = 0, [int]$Y = 0)
$padPort = 7778 + $Slot
$gdbPort = 2345 + $Slot
$title = if ($Slot -eq 0) { "mGBA" } else { "mGBA-$Slot" }
$sfx = if ($Slot -eq 0) { "" } else { "-$Slot" }
$st = if ($State) { "-t $State" } else { "" }
wsl -d Ubuntu -u root -- bash -c "FORK_SLOT=$Slot /root/gdbsrv.sh stop 2>/dev/null; fuser -k $gdbPort/tcp $padPort/tcp 2>/dev/null; echo stopped slot $Slot"
Start-Sleep -Milliseconds 500
Start-Process -FilePath wsl -ArgumentList @('-d', 'Ubuntu', '-u', 'root', '--', 'bash', '-c', "trap '' PIPE; cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=$padPort MGBA_TITLE=$title exec ./mgba -2 $st $Rom > /root/mgba$sfx.log 2>&1") -WindowStyle Hidden
Start-Sleep -Seconds 4
& "$PSScriptRoot\st_place.ps1" -Title $title -X $X -Y $Y | Select-Object -Last 1
Write-Host "running slot $Slot pad $padPort title $title (no gdb)"
