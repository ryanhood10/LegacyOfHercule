# Move a window (title contains -Title) to screen position X, Y so it is not under another session's
# fork window (win_shot.ps1 copies the screen area, so an overlapping window spoils screenshots).
# usage: st_place.ps1 -Title "mGBA-1" -X 1300 -Y 40       (prints every mGBA window's rectangle)
param([string]$Title = "mGBA-1", [int]$X = -1, [int]$Y = -1)
Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public struct RC { public int L, T, R, B; }
public static class WP {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RC r);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[WP]::SetProcessDPIAware() | Out-Null
$target = [IntPtr]::Zero
$cb = [WP+EnumProc]{ param($h, $l)
  if ([WP]::IsWindowVisible($h)) {
    $sb = New-Object Text.StringBuilder 256; [WP]::GetWindowText($h, $sb, 256) | Out-Null; $t = $sb.ToString()
    if ($t -like "*mGBA*") { $r = New-Object RC; [WP]::GetWindowRect($h, [ref]$r) | Out-Null; Write-Host "$t : $($r.L),$($r.T) - $($r.R),$($r.B)" }
    if ($t -like "*$Title*" -and $script:target -eq [IntPtr]::Zero) { $script:target = $h }
  }
  return $true }
[WP]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
if ($X -ge 0 -and $target -ne [IntPtr]::Zero) { [WP]::SetWindowPos($target, [IntPtr]::Zero, $X, $Y, 0, 0, 0x0015) | Out-Null; Write-Host "moved '$Title' to $X,$Y" }
