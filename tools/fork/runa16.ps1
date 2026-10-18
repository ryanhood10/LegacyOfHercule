# Launch the WSL fork on the Android 16 test build (optionally a savestate) through runfork.ps1, so only
# the chosen slot's instance is replaced (the old version ran `pkill -9 -x mgba` and stopped every
# session's fork). usage: tools\fork\runa16.ps1 [-Rom /mnt/f/.../log2-wide.gba] [-State /mnt/f/.../x.ss1] [-Slot n]
param([string]$Rom = "/mnt/f/MeltyCreations/log2/build/a16/log2-wide.gba", [string]$State = "", [int]$Slot = 0)
& (Join-Path $PSScriptRoot "runfork.ps1") $Rom -Wait 4 -Slot $Slot -State $State -Mute
