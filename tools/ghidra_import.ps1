# Import a GBA ROM into a Ghidra project (headless) with the GBA memory map and full auto-analysis.
# Usage: tools\ghidra_import.ps1 roms\log2.gba [ProjectName]
param(
    [Parameter(Mandatory = $true)][string]$Rom,
    [string]$Project = "log2"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$ghidra = Get-ChildItem "$repo\..\tools\ghidra" -Directory | Where-Object Name -like "ghidra_*" | Select-Object -First 1
if (-not $ghidra) { throw "Ghidra not found under tools\ghidra" }

$projDir = Join-Path $repo "build\ghidra"
New-Item -ItemType Directory -Force $projDir | Out-Null
$romPath = (Resolve-Path $Rom).Path
$romName = [IO.Path]::GetFileNameWithoutExtension($romPath)

& "$($ghidra.FullName)\support\analyzeHeadless.bat" $projDir $Project `
    -import $romPath `
    -processor "ARM:LE:32:v4t" `
    -loader BinaryLoader -loader-baseAddr 0x08000000 -loader-blockName ROM `
    -scriptPath "$repo\tools\ghidra" `
    -preScript GbaMemoryMap.java `
    -analysisTimeoutPerFile 3600 `
    -overwrite

Write-Host "Imported $romName into $projDir\$Project.gpr"
