# Burst of screenshots of a window (one process: the Add-Type compile happens once, ~1.5 s, so start it
# before the action). usage: tools\win_burst.ps1 -Prefix build\x\b -Title "mGBA-2 (" [-Count 8] [-Ms 60] [-Delay 0]
# -> <Prefix>0.png ... ; check the title bar in each frame (another window may take the foreground).
param([string]$Prefix, [string]$Title = "mGBA-2 (", [int]$Count = 8, [int]$Ms = 120, [int]$Delay = 0)
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
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
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
if ($h -eq [IntPtr]::Zero) { throw "no window $Title" }
[WT]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds $Delay
for ($i = 0; $i -lt $Count; $i++) {
  $rc = New-Object RECTW; [WT]::GetClientRect($h, [ref]$rc) | Out-Null
  $pt = New-Object POINTW; [WT]::ClientToScreen($h, [ref]$pt) | Out-Null
  $w = $rc.Right - $rc.Left; $hh = $rc.Bottom - $rc.Top
  $bmp = New-Object System.Drawing.Bitmap $w, $hh
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
  $bmp.Save("$Prefix$i.png", [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Start-Sleep -Milliseconds $Ms
}
"burst $Count"
