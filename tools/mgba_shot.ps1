# Screenshot the mGBA (SDL) window to a PNG.  Usage: tools\mgba_shot.ps1 out.png
param([Parameter(Mandatory = $true)][string]$Out)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public struct RECT { public int Left, Top, Right, Bottom; }
public struct POINT { public int X, Y; }
public static class W {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[W]::SetProcessDPIAware() | Out-Null
$p = Get-Process mgba-sdl -ErrorAction Stop | Select-Object -First 1
$h = $p.MainWindowHandle
$rc = New-Object RECT; [W]::GetClientRect($h, [ref]$rc) | Out-Null
$pt = New-Object POINT; [W]::ClientToScreen($h, [ref]$pt) | Out-Null
$w = $rc.Right - $rc.Left; $hgt = $rc.Bottom - $rc.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "saved $Out ($w x $hgt)"
