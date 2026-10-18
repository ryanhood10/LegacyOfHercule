# Restart mGBA on a *copy* of the built ROM so `make` can overwrite build\log2-coop.gba while mGBA runs
# (mGBA keeps the loaded file locked). Copies the savestates too (build\log2-coop.ssN -> log2-run.ssN)
# and loads the co-op Lua helpers plus the TCP command server (tools\mgba_cmd.py) into the new instance.
# Usage: tools\mgba_run.ps1 [-Rom build\log2-coop.gba] [-NoStates]
param([string]$Rom = "build\log2-coop.gba", [switch]$NoStates)
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
Get-Process mGBA -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 800
$run = $Rom.Substring(0, $Rom.Length - 4).Replace('-coop', '-run')
if ($run -eq $Rom.Substring(0, $Rom.Length - 4)) { $run = $run + '-run' }
Copy-Item $Rom "$run.gba" -Force
if (-not $NoStates) {
    $base = $Rom.Substring(0, $Rom.Length - 4)
    Get-ChildItem "$base.ss*" -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item $_.FullName ("$run" + $_.Extension) -Force
    }
}
$loader = Join-Path $root "build\mgba_run_loader.lua"
@"
dofile("$($root.Replace('\', '/'))/tools/mgba/coop.lua")
dofile("$($root.Replace('\', '/'))/tools/mgba/cmd_server.lua")
"@ | Set-Content -Encoding ascii $loader
Start-Process -FilePath 'C:\Program Files\mGBA\mGBA.exe' -ArgumentList @((Resolve-Path "$run.gba").Path) -WorkingDirectory $root
Start-Sleep 6
& "$PSScriptRoot\mgba_script.ps1" ('dofile("' + $loader.Replace('\', '/') + '")')
Start-Sleep 2
python -I tools\mgba_cmd.py 'return "mgba_run: ready"'
