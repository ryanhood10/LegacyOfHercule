# Import a GBA ROM into a Ghidra project (headless) with the GBA memory map, seeded Thumb functions,
# a curated analyzer set, and a CSV export of the resulting function list.
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
$seeds = Join-Path $repo "build\$romName.seeds.txt"
$csv = Join-Path $repo "build\$romName.functions.csv"

python -I "$repo\tools\seed_functions.py" $romPath $seeds
if ($LASTEXITCODE -ne 0) { throw "seed_functions.py failed" }

& "$($ghidra.FullName)\support\analyzeHeadless.bat" $projDir $Project `
    -import $romPath -overwrite -noanalysis `
    -processor "ARM:LE:32:v4t" `
    -loader BinaryLoader -loader-baseAddr 0x08000000 -loader-blockName ROM `
    -scriptPath "$repo\tools\ghidra" `
    -preScript GbaMemoryMap.java `
    -postScript SeedFunctions.java $seeds `
    -postScript ExportFunctions.java $csv

Write-Host "Imported $romName into $projDir\$Project.gpr; functions in $csv"
