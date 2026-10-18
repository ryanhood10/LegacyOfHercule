# Launch the WSL fork on the Android 16 test build (optionally a savestate), attach the persistent gdb
# client, continue. usage: tools\fork\runa16.ps1 [-Rom /mnt/f/.../log2-wide.gba] [-State /mnt/f/.../x.ss1]
param([string]$Rom = "/mnt/f/MeltyCreations/log2-a16/build/a16/log2-wide.gba", [string]$State = "")
wsl -d Ubuntu -u root -- bash -c "pkill -9 -x mgba; pkill -9 -x gdb-multiarch; echo stopped"
Start-Sleep -Milliseconds 500
$st = if ($State) { "-t $State" } else { "" }
Start-Process -FilePath wsl -ArgumentList @('-d', 'Ubuntu', '-u', 'root', '--', 'bash', '-c', "trap '' PIPE; cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=7778 exec ./mgba -2 -g $st $Rom > /root/mgba-a16.log 2>&1") -WindowStyle Hidden
Start-Sleep -Seconds 3
$n = 0; while (-not (wsl -d Ubuntu -u root -- bash -c "ss -ltn | grep -q 2345 && echo y") -and $n -lt 20) { Start-Sleep -Seconds 1; $n++ }
wsl -d Ubuntu -u root -- bash -c "/root/gdbsrv.sh start > /dev/null; /root/waitlog.sh 20 'Remote debugging' 1 > /dev/null; /root/gdbsrv.sh cmd continue; echo running"
