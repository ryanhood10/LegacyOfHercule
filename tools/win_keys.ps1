# Hold keys in any window found by title (e.g. the WSLg "mGBA (Ubuntu)" window).
# Usage: tools\win_keys.ps1 "RIGHT:2000 WAIT:500 X:100" -Title "mGBA (Ubuntu)"
# Same step syntax as mgba_input.ps1: NAME:ms, WAIT:ms, A+B:ms for chords. mGBA SDL defaults: arrows,
# X=A, Z=B, A=L, S=R, Enter=Start, Backspace=Select.
param([Parameter(Mandatory = $true)][string]$Script, [Parameter(Mandatory = $true)][string]$Title)
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public static class WK {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint mapType);
  public static IntPtr Find(string needle) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => { if (!IsWindowVisible(h)) return true; var sb = new StringBuilder(256); GetWindowText(h, sb, 256);
      if (sb.ToString().IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0) { found = h; return false; } return true; }, IntPtr.Zero);
    return found;
  }
}
"@
$vk = @{ UP = 0x26; DOWN = 0x28; LEFT = 0x25; RIGHT = 0x27; X = 0x58; Z = 0x5A; A = 0x41; S = 0x53; N = 0x4E; O = 0x4F;
         I = 0x49; J = 0x4A; K = 0x4B; L = 0x4C; U = 0x55; M = 0x4D; ENTER = 0x0D; BACKSPACE = 0x08; ESC = 0x1B; F1 = 0x70; F2 = 0x71 }
$h = [WK]::Find($Title)
if ($h -eq [IntPtr]::Zero) { throw "no window with title containing '$Title'" }
[WK]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 300
function Press([int]$code, [bool]$down) {
    $scan = [byte][WK]::MapVirtualKey($code, 0)
    $ext = if ($code -in 0x25, 0x26, 0x27, 0x28) { 1 } else { 0 }
    [WK]::keybd_event([byte]$code, $scan, ($(if ($down) { 0 } else { 2 }) -bor $ext), [UIntPtr]::Zero)
}
foreach ($step in $Script.Split(' ', [StringSplitOptions]::RemoveEmptyEntries)) {
    $cut = $step.LastIndexOf(':'); $name = $step.Substring(0, $cut); $ms = [int]$step.Substring($cut + 1)
    if ($name -eq 'WAIT') { Start-Sleep -Milliseconds $ms; continue }
    $codes = @($name.Split('+') | ForEach-Object { [int]$vk[$_.ToUpper()] })
    foreach ($c in $codes) { Press $c $true }
    Start-Sleep -Milliseconds $ms
    foreach ($c in $codes) { Press $c $false }
    Start-Sleep -Milliseconds 60
}
