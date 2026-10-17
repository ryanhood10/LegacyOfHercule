# Run a line of Lua in an mGBA window's Scripting console (opens Tools > Scripting if needed).
# Usage: tools\mgba_script.ps1 'dofile("F:/MeltyCreations/log2/tools/mgba/coop.lua")' [-Window N]
# Keystrokes go to that mGBA instance only; do not drive the game window at the same time.
param([Parameter(Mandatory = $true)][string]$Lua, [int]$Window = 0)
. "$PSScriptRoot\mgba_window.ps1"
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public struct PT { public int X, Y; }
public struct RC { public int Left, Top, Right, Bottom; }
public static class KS {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref PT p);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RC r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, int dx, int dy, uint data, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[KS]::SetProcessDPIAware() | Out-Null
function Click([IntPtr]$h, [int]$x, [int]$y) {   # left-click at client coordinates of $h
    $p = New-Object PT; $p.X = $x; $p.Y = $y; [KS]::ClientToScreen($h, [ref]$p) | Out-Null
    [KS]::SetCursorPos($p.X, $p.Y) | Out-Null; Start-Sleep -Milliseconds 100
    [KS]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 50; [KS]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
}
$main = Get-MgbaWindow -Index $Window
$pid_ = Get-WindowPid $main
$con = $null
try { $con = Get-MgbaWindow -Title 'Scripting*' -ProcessId $pid_ } catch {}
if (-not $con) {
    # The Qt menu bar ignores Alt mnemonics here, so open Tools > Scripting... by mouse (positions as laid
    # out by mGBA 0.10.5 at 100 % scaling: Tools at x=252 on the menu bar, Scripting... the fifth entry).
    [KS]::SetForegroundWindow($main) | Out-Null; Start-Sleep -Milliseconds 300
    Click $main 252 13; Start-Sleep -Milliseconds 600
    Click $main 298 137; Start-Sleep -Milliseconds 1200
    $con = Get-MgbaWindow -Title 'Scripting*' -ProcessId $pid_
}
[KS]::SetForegroundWindow($con) | Out-Null; Start-Sleep -Milliseconds 300
$rc = New-Object RC; [KS]::GetClientRect($con, [ref]$rc) | Out-Null
Click $con ([int](($rc.Right - $rc.Left) / 2)) ($rc.Bottom - 50)     # the one-line input box above "Run"
Start-Sleep -Milliseconds 200
$escaped = ($Lua.ToCharArray() | ForEach-Object { if ('+^%~(){}[]'.Contains($_)) { "{$_}" } else { "$_" } }) -join ''
[System.Windows.Forms.SendKeys]::SendWait($escaped)
Start-Sleep -Milliseconds 100
[System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
Start-Sleep -Milliseconds 300
Write-Host "ran in window $Window console: $Lua"
