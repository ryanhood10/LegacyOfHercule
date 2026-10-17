# Hold keys in an mGBA window.  Usage: tools\mgba_input.ps1 "ENTER:150 WAIT:500 X:100 RIGHT:1000" [-Window N]
# Names: UP DOWN LEFT RIGHT X Z A S ENTER BACKSPACE N O and CTRL+<key>; WAIT:<ms> pauses.
# mGBA defaults: X=A Z=B A=L S=R Enter=Start Backspace=Select. Several keys at once: "A+S:1500".
param([Parameter(Mandatory = $true)][string]$Script, [int]$Window = 0, [string]$Title = "")
. "$PSScriptRoot\mgba_window.ps1"
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class K {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
}
"@
$vk = @{ UP = 0x26; DOWN = 0x28; LEFT = 0x25; RIGHT = 0x27; X = 0x58; Z = 0x5A; A = 0x41; S = 0x53; N = 0x4E; O = 0x4F;
         I = 0x49; J = 0x4A; K = 0x4B; L = 0x4C; U = 0x55; M = 0x4D;
         ENTER = 0x0D; BACKSPACE = 0x08; CTRL = 0x11; SHIFT = 0x10; F1 = 0x70; ESC = 0x1B }
$h = Get-MgbaWindow -Index $Window -Title $Title
[K]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 300
function Press([int]$code, [bool]$down) {
    $scan = [byte][K]::MapVirtualKey($code, 0)
    $ext = if ($code -in 0x25, 0x26, 0x27, 0x28) { 1 } else { 0 }
    [K]::keybd_event([byte]$code, $scan, ($(if ($down) { 0 } else { 2 }) -bor $ext), [UIntPtr]::Zero)
}
foreach ($step in $Script.Split(' ', [StringSplitOptions]::RemoveEmptyEntries)) {
    $cut = $step.LastIndexOf(':'); $name = $step.Substring(0, $cut); $ms = [int]$step.Substring($cut + 1)
    if ($name -eq 'WAIT') { Start-Sleep -Milliseconds $ms; continue }
    if ($name -like 'TYPE=*') { [System.Windows.Forms.SendKeys]::SendWait($name.Substring(5)); Start-Sleep -Milliseconds $ms; continue }
    $codes = @($name.Split('+') | ForEach-Object { [int]$vk[$_.ToUpper()] })
    foreach ($c in $codes) { Press $c $true; Start-Sleep -Milliseconds 20 }
    Start-Sleep -Milliseconds $ms
    [array]::Reverse($codes); foreach ($c in $codes) { Press $c $false; Start-Sleep -Milliseconds 20 }
    Start-Sleep -Milliseconds 60
}
