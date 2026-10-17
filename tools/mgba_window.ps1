# Shared helper: find mGBA top-level windows (SDL or Qt). Dot-source this file, then call Get-MgbaWindow -Index N.
Add-Type @"
using System; using System.Collections.Generic; using System.Runtime.InteropServices; using System.Text;
public static class WinEnum {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  public static List<IntPtr> ForPids(HashSet<uint> pids) {
    var list = new List<IntPtr>();
    EnumWindows((h, l) => {
      uint pid; GetWindowThreadProcessId(h, out pid);
      if (pids.Contains(pid) && IsWindowVisible(h) && GetWindowTextLength(h) > 0) list.Add(h);
      return true; }, IntPtr.Zero);
    return list;
  }
  public static string Title(IntPtr h) { var sb = new StringBuilder(256); GetWindowText(h, sb, 256); return sb.ToString(); }
}
"@
function Get-MgbaWindow([int]$Index = 0, [string]$Title = '') {
    $pids = New-Object 'System.Collections.Generic.HashSet[uint32]'
    Get-Process mgba-sdl, mGBA -ErrorAction SilentlyContinue | ForEach-Object { [void]$pids.Add([uint32]$_.Id) }
    $all = [WinEnum]::ForPids($pids)
    if ($Title) { $wins = $all | Where-Object { [WinEnum]::Title($_) -like $Title }; $Index = 0 }
    else { $wins = $all | Where-Object { [WinEnum]::Title($_) -like 'mGBA*' } }
    # EnumWindows returns z-order (front first); sort by handle for a stable "creation order" instead
    $wins = @($wins | Sort-Object { [int64]$_ })
    if ($wins.Count -le $Index) { throw "mGBA window $Index not found ($($wins.Count) windows)" }
    return $wins[$Index]
}
