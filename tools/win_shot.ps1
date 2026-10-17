# Screenshot any visible top-level window whose title contains the given text (e.g. the WSLg
# "mGBA (Ubuntu)" window).  Usage: tools\win_shot.ps1 out.png -Title "mGBA (Ubuntu)"
param([Parameter(Mandatory = $true)][string]$Out, [Parameter(Mandatory = $true)][string]$Title)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Collections.Generic; using System.Runtime.InteropServices; using System.Text;
public struct RECTW { public int Left, Top, Right, Bottom; }
public struct POINTW { public int X, Y; }
public static class WT {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECTW r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINTW p);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  public static IntPtr Find(string needle) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => { if (!IsWindowVisible(h)) return true; var sb = new StringBuilder(256); GetWindowText(h, sb, 256);
      if (sb.ToString().IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0) { found = h; return false; } return true; }, IntPtr.Zero);
    return found;
  }
}
"@
[WT]::SetProcessDPIAware() | Out-Null
$h = [WT]::Find($Title)
if ($h -eq [IntPtr]::Zero) { throw "no window with title containing '$Title'" }
[WT]::SetForegroundWindow($h) | Out-Null; Start-Sleep -Milliseconds 250
$rc = New-Object RECTW; [WT]::GetClientRect($h, [ref]$rc) | Out-Null
$pt = New-Object POINTW; [WT]::ClientToScreen($h, [ref]$pt) | Out-Null
$w = $rc.Right - $rc.Left; $hh = $rc.Bottom - $rc.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "saved $Out ($w x $hh)"
