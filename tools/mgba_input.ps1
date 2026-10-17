# Hold keys in the mGBA (SDL) window.  Usage: tools\mgba_input.ps1 "ENTER:150 WAIT:500 X:100 RIGHT:1000"
# Names: UP DOWN LEFT RIGHT X Z A S ENTER BACKSPACE; WAIT:<ms> pauses. mGBA defaults: X=A Z=B A=L S=R.
param([Parameter(Mandatory = $true)][string]$Script)
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class K {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
}
"@
$vk = @{ UP = 0x26; DOWN = 0x28; LEFT = 0x25; RIGHT = 0x27; X = 0x58; Z = 0x5A; A = 0x41; S = 0x53; ENTER = 0x0D; BACKSPACE = 0x08 }
$p = Get-Process mgba-sdl, mGBA -ErrorAction SilentlyContinue | Where-Object MainWindowHandle -ne 0 | Select-Object -First 1
[K]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 300
foreach ($step in $Script.Split(' ', [StringSplitOptions]::RemoveEmptyEntries)) {
    $name, $ms = $step.Split(':'); $ms = [int]$ms
    if ($name -eq 'WAIT') { Start-Sleep -Milliseconds $ms; continue }
    $code = [byte]$vk[$name.ToUpper()]; $scan = [byte][K]::MapVirtualKey($code, 0)
    $ext = if ($code -in 0x25, 0x26, 0x27, 0x28) { 1 } else { 0 }
    [K]::keybd_event($code, $scan, $ext, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds $ms
    [K]::keybd_event($code, $scan, $ext -bor 2, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 60
}
