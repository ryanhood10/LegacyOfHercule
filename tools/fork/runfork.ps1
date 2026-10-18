# Launch the WSL mGBA fork on a ROM with the GDB stub, attach the persistent gdb client, let it run.
# usage: runfork.ps1 <rom path under /mnt/f/...> [seconds to wait after boot]
param([string]$Rom, [int]$Wait = 14)
wsl -d Ubuntu -u root -- bash -c "pkill -9 -x mgba; pkill -9 -x gdb-multiarch; pkill -x sleep; echo stopped"
Start-Sleep -Milliseconds 500
Start-Process -FilePath wsl -ArgumentList @('-d', 'Ubuntu', '-u', 'root', '--', 'bash', '-c', "trap '' PIPE; cd /root/mgba-build/sdl && MGBA_P2=1 MGBA_P2_PORT=7778 exec ./mgba -2 -g $Rom > /root/mgba-party.log 2>&1") -WindowStyle Hidden
Start-Sleep -Seconds 3
while (-not (wsl -d Ubuntu -u root -- bash -c "ss -ltn | grep -q 2345 && echo y")) { Start-Sleep -Seconds 1 }
wsl -d Ubuntu -u root -- bash -c "/root/gdbsrv.sh start > /dev/null; /root/waitlog.sh 20 'Remote debugging' 1 > /dev/null; /root/gdbsrv.sh cmd continue; echo running"
Start-Sleep -Seconds $Wait
