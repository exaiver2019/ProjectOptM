# =====================================================================
#  PROJECT OPTM - app source
#
#  This file IS the app. Build.bat packs it into dist\ProjectOptM.exe
#  (plus a ProjectOptM.bat for older installs). Edit this file, then run
#  Build.bat - or Publish-Release.bat, which builds and publishes.
#
#  Detects your CPU, GPU, RAM and display on startup and adapts to them.
#  Game profiles live in %APPDATA%\ProjectOptM\profiles.ini.
# =====================================================================

# ======================= VERSION & UPDATES =======================
# Bump $AppVersion for every release (1.0.0 -> 1.0.1 -> 1.1.0 ...).
# $UpdateRepo is the public GitHub repo your releases live in, as "username/repo".
# Leave it empty to turn update checks off.
$AppVersion = '1.1.0'
$UpdateRepo = 'exaiver2019/ProjectOptM'

# Default profiles file (written to %APPDATA%\ProjectOptM\profiles.ini on first run)
$DefaultIni = @'
; ================================================================
;                 PROJECT OPTM  -  GAME PROFILES
;                   (profiles format v2 - games v12)
; ================================================================
;
;   Save this file (Ctrl+S) and Project OptM reloads it by itself.
;   Lines starting with ; are notes and are ignored.
;
;   ADD A GAME:  copy any block below and change the [name] + exe.
;   Find the exe name in Task Manager > Details while it's running.
;
;   Delete this file to get the default profiles back.
;
; ----------------------------------------------------------------
;   OPTION        WHAT IT DOES                        DEFAULT
; ----------------------------------------------------------------
;   exe           Process name(s), comma separated     (required)
;   anticheat     yes = don't touch the game itself,    no
;                 only system tweaks. For games with
;                 kernel anti-cheat.
;   priority      Normal / AboveNormal / High           AboveNormal
;   cores         Best  = picked from your CPU          Best
;                 Other = non-V-Cache CCD (X3D only)
;                 All   = no pinning
;   ramcleanup    Minutes between standby RAM clears    0 (off)
;                 (adjusted for how much RAM you have)
;   boost         Extra processes set to High while     none
;                 playing, comma separated
;   launch_priority  Priority Windows itself gives the     off
;                 game when it starts. Nothing touches
;                 the running game - safe for anti-cheat.
;   close         Apps to close when this game starts   none
;                 (not reopened), comma separated
;   keep          Launchers this game needs - never      none
;                 closed for it. Groups: steam, epic,
;                 riot, battlenet, ea, oculus, xbox
;                 (or any app name)
;   ecoqos_off    yes = stop Windows' efficiency mode    yes
;                 from throttling the game (skipped
;                 for anti-cheat games)
; ================================================================


; ----------------------------------------------------------------
;   GENERAL
; ----------------------------------------------------------------

[Settings]
check_every          = 3      ; seconds between game checks
pause_updates        = yes    ; pause Windows Update downloads while playing
power_plan           = auto   ; auto = High performance while playing, except on
                              ;        dual-CCD X3D (those need Balanced)
                              ; high = always switch  |  off = never touch
force_dedicated_gpu  = yes    ; if you have an iGPU too, lock games to the real GPU
cleanup_on_launch    = yes    ; clear standby RAM once when any game starts
pause_services       = SysMain, WSearch   ; paused while playing, restarted after
panic_hotkey         = yes    ; Ctrl+Alt+End instantly undoes everything

; Apps to close when ANY game starts. Launcher groups work here too
; (steam, epic, riot, battlenet, ea, oculus, xbox) - a game's "keep"
; list protects the launchers it needs. Example:
; close        = epic, riot, ea, wallpaper64
reopen_closed        = yes    ; reopen those apps when the game ends

; Hardware-specific (AMD / Intel / NVIDIA)
background_cores     = auto   ; while playing, move background apps off the game's cores:
                              ; Intel hybrid = E-cores, AMD dual-CCD X3D = the non-V-Cache CCD
                              ; off = never
vendor_apps          = yes    ; also lower your GPU maker's helper apps (Radeon Software,
                              ; NVIDIA app/overlay, Intel Graphics Software)

; Apps set to low priority while you play (restored after).
; Add more on extra "background =" lines.
background    = chrome, msedge, firefox, opera, brave, zen
background    = Discord, Spotify, steamwebhelper
background    = EpicGamesLauncher, Battle.net, OneDrive, Teams, ms-teams


; ----------------------------------------------------------------
;   ANTI-CHEAT GAMES  (game process is never touched)
; ----------------------------------------------------------------

[Escape from Tarkov]
exe              = EscapeFromTarkov
anticheat        = yes
launch_priority  = AboveNormal
ramcleanup       = 15

[Black Ops 7 / Warzone]
exe              = cod, cod25, BlackOps7
anticheat        = yes
launch_priority  = AboveNormal
keep             = steam, battlenet

[Escape from Tarkov: Arena]
exe              = EscapeFromTarkovArena
anticheat        = yes
launch_priority  = AboveNormal
ramcleanup       = 15

[Fortnite]
exe              = FortniteClient-Win64-Shipping, FortniteClient-Win64-Shipping_EAC, FortniteClient-Win64-Shipping_BE
anticheat        = yes
launch_priority  = AboveNormal
keep             = epic

[Valorant]
exe              = VALORANT-Win64-Shipping, VALORANT
anticheat        = yes
keep             = riot

[Rust]
exe              = RustClient
anticheat        = yes
launch_priority  = AboveNormal
keep             = steam
ramcleanup       = 20

[Battlefield 6]
exe              = bf6
anticheat        = yes
launch_priority  = AboveNormal
keep             = steam, ea

[Black Ops Cold War]
exe              = BlackOpsColdWar
anticheat        = yes
launch_priority  = AboveNormal
keep             = battlenet, steam

[Helldivers 2]
exe              = helldivers2
anticheat        = yes
keep             = steam

[Roblox]
exe              = RobloxPlayerBeta
anticheat        = yes

[Forza Horizon 6]
exe              = ForzaHorizon6
anticheat        = yes          ; unsure if it has one - safe mode just in case
keep             = steam


; ----------------------------------------------------------------
;   SINGLE-PLAYER
; ----------------------------------------------------------------

[Cyberpunk 2077]
exe           = Cyberpunk2077
priority      = AboveNormal
cores         = Best

[DOOM: The Dark Ages]
exe           = DOOMTheDarkAges, DOOMTheDarkAges_x64
priority      = AboveNormal
cores         = Best

[S.T.A.L.K.E.R. 2]
exe           = Stalker2-Win64-Shipping, Stalker2-WinGDK-Shipping, Stalker2
priority      = AboveNormal
cores         = Best
ramcleanup    = 20

[Halloween: The Game]
exe           = Halloween-Win64-Shipping, Halloween
priority      = AboveNormal
cores         = Best

[Counter-Strike 2]
exe              = cs2
priority         = AboveNormal
cores            = Best
keep             = steam
; playing on FACEIT? set anticheat = yes

[Resident Evil Requiem]
exe              = re9
priority         = AboveNormal
cores            = Best
keep             = steam


; ----------------------------------------------------------------
;   VR
; ----------------------------------------------------------------

[Into the Radius 2 (VR)]
exe           = IntoTheRadius2-Win64-Shipping, IntoTheRadius2
priority      = AboveNormal
cores         = Best
boost         = OVRServer_x64

[Bonelab (VR)]
exe              = BONELAB_Steam_Windows64, BONELAB_Oculus_Windows64
priority         = AboveNormal
cores            = Best
boost            = OVRServer_x64
keep             = steam, oculus

[Boneworks (VR)]
exe              = BONEWORKS_Oculus_Windows64, Boneworks_Steam_Windows64
priority         = AboveNormal
cores            = Best
boost            = OVRServer_x64
keep             = steam, oculus


; ----------------------------------------------------------------
;   SANDBOX
; ----------------------------------------------------------------

; also covers Project Zomboid - it runs on the same javaw
[Minecraft Java]
exe           = javaw
priority      = AboveNormal
cores         = Best
'@

# Extra games - appended to existing profile files that don't have them yet
$NewGamesV12 = @'
[Escape from Tarkov: Arena]
exe              = EscapeFromTarkovArena
anticheat        = yes
launch_priority  = AboveNormal
ramcleanup       = 15

[Fortnite]
exe              = FortniteClient-Win64-Shipping, FortniteClient-Win64-Shipping_EAC, FortniteClient-Win64-Shipping_BE
anticheat        = yes
launch_priority  = AboveNormal
keep             = epic

[Valorant]
exe              = VALORANT-Win64-Shipping, VALORANT
anticheat        = yes
keep             = riot

[Rust]
exe              = RustClient
anticheat        = yes
launch_priority  = AboveNormal
keep             = steam
ramcleanup       = 20

[Battlefield 6]
exe              = bf6
anticheat        = yes
launch_priority  = AboveNormal
keep             = steam, ea

[Black Ops Cold War]
exe              = BlackOpsColdWar
anticheat        = yes
launch_priority  = AboveNormal
keep             = battlenet, steam

[Helldivers 2]
exe              = helldivers2
anticheat        = yes
keep             = steam

[Roblox]
exe              = RobloxPlayerBeta
anticheat        = yes

[Forza Horizon 6]
exe              = ForzaHorizon6
anticheat        = yes          ; unsure if it has one - safe mode just in case
keep             = steam

[Counter-Strike 2]
exe              = cs2
priority         = AboveNormal
cores            = Best
keep             = steam
; playing on FACEIT? set anticheat = yes

[Resident Evil Requiem]
exe              = re9
priority         = AboveNormal
cores            = Best
keep             = steam

[Bonelab (VR)]
exe              = BONELAB_Steam_Windows64, BONELAB_Oculus_Windows64
priority         = AboveNormal
cores            = Best
boost            = OVRServer_x64
keep             = steam, oculus

[Boneworks (VR)]
exe              = BONEWORKS_Oculus_Windows64, Boneworks_Steam_Windows64
priority         = AboveNormal
cores            = Best
boost            = OVRServer_x64
keep             = steam, oculus
'@

# ======================= (no need to edit below) =======================

$ErrorActionPreference = 'Continue'
Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase, System.Windows.Forms, System.Drawing

try {

# --- Only one copy at a time ---
$script:Mutex = [System.Threading.Mutex]::new($false, 'Local\ProjectOptMSingleInstance')
$waitMs = if ($env:OPTM_RESTART) { 15000 } else { 0 }
$gotLock = $false
try { $gotLock = $script:Mutex.WaitOne($waitMs) } catch [System.Threading.AbandonedMutexException] { $gotLock = $true }
if (-not $gotLock) {
  [void][System.Windows.MessageBox]::Show('Project OptM is already running. Look for its icon in the system tray.', 'Project OptM')
  exit
}

# Leftover from a self-update (the previous exe is renamed aside while it's running)
if ("$env:GO_SELF" -like '*.exe') { Remove-Item -LiteralPath "$env:GO_SELF.old" -Force -ErrorAction SilentlyContinue }

# --- Profiles file ---
$ProfilesPath = Join-Path $env:APPDATA 'ProjectOptM\profiles.ini'

function Initialize-ProfilesFile {
  if ((Test-Path -LiteralPath $ProfilesPath) -and -not (Select-String -LiteralPath $ProfilesPath -Pattern 'profiles format v2' -Quiet)) {
    $bak = Join-Path (Split-Path $ProfilesPath) 'profiles.old.ini'
    Move-Item -LiteralPath $ProfilesPath -Destination $bak -Force
    $script:ProfilesUpgraded = $true
  }
  if ((Test-Path -LiteralPath $ProfilesPath) -and -not (Select-String -LiteralPath $ProfilesPath -Pattern 'pause_services|NEW IN 1\.1|NEW OPTIONS' -Quiet)) {
    $note = @(
      '', '',
      '; ----------------------------------------------------------------',
      ';   NEW OPTIONS  (already on by default - add these to change them)',
      '; ----------------------------------------------------------------',
      ';   In [Settings]:',
      ';     pause_services = SysMain, WSearch   paused while playing',
      ';     panic_hotkey   = yes                Ctrl+Alt+End undoes everything',
      ';   In a game block:',
      ";     ecoqos_off     = yes                stop Windows' efficiency mode",
      ';                                         from throttling the game'
    ) -join "`r`n"
    [IO.File]::AppendAllText($ProfilesPath, $note)
  }
  if ((Test-Path -LiteralPath $ProfilesPath) -and -not (Select-String -LiteralPath $ProfilesPath -Pattern 'ADDED IN 1\.2|MORE GAMES \(added|games v12' -Quiet)) {
    $cur = [IO.File]::ReadAllText($ProfilesPath)
    $add = @()
    foreach ($block in ($NewGamesV12 -split "(?m)(?=^\[)")) {
      if ($block -notmatch '(?m)^exe\s*=\s*(.+)$') { continue }
      $exes = @($Matches[1] -replace '\s+[;#].*$', '' -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
      $have = $false
      foreach ($e in $exes) { if ($cur -match "(?im)^\s*exe\s*=.*\b$([regex]::Escape($e))\b") { $have = $true } }
      if (-not $have) { $add += $block.Trim() }
    }
    $note = "`r`n`r`n; ----------------------------------------------------------------`r`n;   MORE GAMES (added by an update)`r`n; ----------------------------------------------------------------`r`n"
    if ($add.Count -gt 0) { $note += "`r`n" + (($add -join "`r`n`r`n") -replace "`r?`n", "`r`n") + "`r`n" }
    $note += "`r`n;   New option for any game:  keep = steam, epic, riot, battlenet, ea, oculus, xbox`r`n;   (launchers that game needs - never closed for it)`r`n"
    [IO.File]::AppendAllText($ProfilesPath, $note)
    $script:GamesAdded = $add.Count
  }
  if ((Test-Path -LiteralPath $ProfilesPath) -and -not (Select-String -LiteralPath $ProfilesPath -Pattern 'background_cores' -Quiet)) {
    $note = @(
      '', '',
      '; ----------------------------------------------------------------',
      ';   HARDWARE OPTIONS  (already on - add these to [Settings] to change them)',
      '; ----------------------------------------------------------------',
      ';     background_cores = auto   move background apps off the game''s cores',
      ';                               (Intel = E-cores, AMD X3D = other CCD) | off',
      ';     vendor_apps      = yes    lower AMD / NVIDIA / Intel helper apps too'
    ) -join "`r`n"
    [IO.File]::AppendAllText($ProfilesPath, $note)
  }
  if (-not (Test-Path -LiteralPath $ProfilesPath)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $ProfilesPath) | Out-Null
    [IO.File]::WriteAllText($ProfilesPath, ($DefaultIni -replace "`r?`n", "`r`n"))
  }
}

function Read-ProfilesFile {
  $warn = @()
  $listKeys = @('exe', 'boost', 'background', 'close', 'pause_services', 'keep')
  $sections = [ordered]@{}
  $cur = $null
  foreach ($raw in [IO.File]::ReadAllLines($ProfilesPath)) {
    $line = $raw.Trim()
    if ($line -eq '' -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }
    $line = ($line -replace '\s+[;#].*$', '').Trim()
    if ($line -match '^\[(.+)\]$') { $cur = $Matches[1].Trim(); if (-not $sections.Contains($cur)) { $sections[$cur] = @{} }; continue }
    if ($cur -and $line -match '^([^=]+)=(.*)$') {
      $k = $Matches[1].Trim().ToLower(); $v = $Matches[2].Trim()
      if ($listKeys -contains $k -and $sections[$cur].ContainsKey($k)) { $sections[$cur][$k] += ",$v" }
      else { $sections[$cur][$k] = $v }
    }
  }
  $split = { param($v) @("$v" -split ',' | ForEach-Object { ($_.Trim()) -replace '\.exe$', '' } | Where-Object { $_ }) }

  $poll = 3; $bg = @()
  $yes = { param($v, $def) if ("$v" -eq '') { $def } else { "$v" -match '^(yes|true|on|1)$' } }
  $set = @{ PauseUpdates = $true; PowerPlan = 'auto'; ForceGpu = $true; CleanupOnLaunch = $true; Close = @(); PauseServices = @('SysMain', 'WSearch'); PanicHotkey = $true; ReopenClosed = $true; BackgroundCores = 'auto'; VendorApps = $true }
  $profs = @()
  foreach ($name in $sections.Keys) {
    $sec = $sections[$name]
    if ($name -eq 'Settings') {
      if ($sec.check_every) { $n = 0; if ([int]::TryParse($sec.check_every, [ref]$n) -and $n -ge 1 -and $n -le 30) { $poll = $n } else { $warn += "check_every '$($sec.check_every)' isn't 1-30, using 3" } }
      $bg = & $split $sec.background
      $set.PauseUpdates    = & $yes $sec.pause_updates $true
      $set.ForceGpu        = & $yes $sec.force_dedicated_gpu $true
      $set.CleanupOnLaunch = & $yes $sec.cleanup_on_launch $true
      $set.Close           = & $split $sec.close
      if ($sec.ContainsKey('pause_services')) { $set.PauseServices = @(& $split $sec.pause_services | Where-Object { $_ -notmatch '^(no|off|none)$' }) }
      $set.PanicHotkey     = & $yes $sec.panic_hotkey $true
      $set.ReopenClosed    = & $yes $sec.reopen_closed $true
      $set.VendorApps      = & $yes $sec.vendor_apps $true
      if ($sec.background_cores) {
        if ($sec.background_cores -match '^(auto|off)$') { $set.BackgroundCores = $sec.background_cores.ToLower() }
        else { $warn += "background_cores '$($sec.background_cores)' unknown, using auto" }
      }
      if ($sec.power_plan) {
        if ($sec.power_plan -match '^(auto|high|off)$') { $set.PowerPlan = $sec.power_plan.ToLower() }
        else { $warn += "power_plan '$($sec.power_plan)' unknown, using auto" }
      }
      continue
    }
    $exe = & $split $sec.exe
    if ($exe.Count -eq 0) { $warn += "[$name] has no exe - skipped"; continue }
    $p = @{ Name = $name; Exe = $exe }
    $p.AntiCheat = "$($sec.anticheat)" -match '^(yes|true|on|1)$'
    $p.Priority = switch -Regex ("$($sec.priority)".Replace(' ', '')) {
      '^$'            { 'AboveNormal' }
      '^normal$'      { 'Normal' }
      '^abovenormal$' { 'AboveNormal' }
      '^high$'        { 'High' }
      default         { $warn += "[$name] priority '$($sec.priority)' unknown, using AboveNormal"; 'AboveNormal' }
    }
    $p.Cores = switch -Regex ("$($sec.cores)") {
      '^$'                  { 'Best' }
      '^(best|vcache)$'     { 'Best' }
      '^(other|frequency)$' { 'Other' }
      '^all$'               { 'All' }
      default               { $warn += "[$name] cores '$($sec.cores)' unknown, using Best"; 'Best' }
    }
    $n = 0
    if (-not $sec.ramcleanup) { $p.PurgeMins = 0 }
    elseif ([int]::TryParse($sec.ramcleanup, [ref]$n) -and $n -ge 0) { $p.PurgeMins = $n }
    else { $p.PurgeMins = 0; $warn += "[$name] ramcleanup '$($sec.ramcleanup)' isn't a number, turned off" }
    $p.Boost = & $split $sec.boost
    $p.Close = & $split $sec.close
    $p.EcoQosOff = & $yes $sec.ecoqos_off $true
    $p.Keep = & $split $sec.keep
    $p.LaunchPriority = switch -Regex ("$($sec.launch_priority)".Replace(' ', '')) {
      '^(|off|no|none)$' { $null }
      '^normal$'         { 'Normal' }
      '^abovenormal$'    { 'AboveNormal' }
      '^high$'           { 'High' }
      default            { $warn += "[$name] launch_priority '$($sec.launch_priority)' unknown, turned off"; $null }
    }
    $profs += $p
  }
  return @{ Profiles = $profs; Background = $bg; Poll = $poll; Settings = $set; Warnings = $warn }
}

function Import-Profiles {
  Initialize-ProfilesFile
  $r = Read-ProfilesFile
  $script:Profiles       = $r.Profiles
  $script:BackgroundApps = $r.Background
  $script:PollSeconds    = $r.Poll
  $script:Settings       = $r.Settings
  $script:ProfileWarnings = $r.Warnings
  $script:ProfilesStamp  = (Get-Item -LiteralPath $ProfilesPath).LastWriteTimeUtc
}

Import-Profiles

# --- Native helpers: standby RAM purge + dark title bar ---
if (-not ('GameOptMem' -as [type])) {
  Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class GameOptMem {
  [StructLayout(LayoutKind.Sequential, Pack = 4)]
  struct TOKEN_PRIV { public int Count; public long Luid; public int Attr; }
  [DllImport("ntdll.dll")] static extern uint NtSetSystemInformation(int infoClass, ref int info, int length);
  [DllImport("advapi32.dll", SetLastError = true)] static extern bool OpenProcessToken(IntPtr h, uint access, out IntPtr token);
  [DllImport("advapi32.dll", SetLastError = true)] static extern bool LookupPrivilegeValue(string sys, string name, out long luid);
  [DllImport("advapi32.dll", SetLastError = true)] static extern bool AdjustTokenPrivileges(IntPtr token, bool disableAll, ref TOKEN_PRIV state, int len, IntPtr prev, IntPtr retLen);
  [DllImport("kernel32.dll")] static extern IntPtr GetCurrentProcess();
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [DllImport("dwmapi.dll")] static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int val, int size);
  public static uint PurgeStandby() {
    IntPtr token;
    if (!OpenProcessToken(GetCurrentProcess(), 0x28, out token)) return 0xFFFFFFFF;
    long luid;
    LookupPrivilegeValue(null, "SeProfileSingleProcessPrivilege", out luid);
    TOKEN_PRIV tp = new TOKEN_PRIV();
    tp.Count = 1; tp.Luid = luid; tp.Attr = 2;
    AdjustTokenPrivileges(token, false, ref tp, 0, IntPtr.Zero, IntPtr.Zero);
    CloseHandle(token);
    int cmd = 4; // MemoryPurgeStandbyList
    return NtSetSystemInformation(80, ref cmd, 4);
  }
  [DllImport("shell32.dll")] static extern int SetCurrentProcessExplicitAppUserModelID([MarshalAs(UnmanagedType.LPWStr)] string appId);
  // Gives the app its own taskbar identity, so Windows shows OptM's icon instead of PowerShell's
  public static void SetAppId(string appId) { SetCurrentProcessExplicitAppUserModelID(appId); }
  public static void DarkTitleBar(IntPtr hwnd) {
    int on = 1;
    DwmSetWindowAttribute(hwnd, 20, ref on, 4);
  }
}

public static class DisplayInfo {
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
  public struct DEVMODE {
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmDeviceName;
    public short dmSpecVersion; public short dmDriverVersion; public short dmSize; public short dmDriverExtra;
    public int dmFields; public int dmPositionX; public int dmPositionY; public int dmDisplayOrientation; public int dmDisplayFixedOutput;
    public short dmColor; public short dmDuplex; public short dmYResolution; public short dmTTOption; public short dmCollate;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string dmFormName;
    public short dmLogPixels; public int dmBitsPerPel; public int dmPelsWidth; public int dmPelsHeight;
    public int dmDisplayFlags; public int dmDisplayFrequency;
    public int dmICMMethod; public int dmICMIntent; public int dmMediaType; public int dmDitherType;
    public int dmReserved1; public int dmReserved2; public int dmPanningWidth; public int dmPanningHeight;
  }
  [DllImport("user32.dll", CharSet = CharSet.Ansi)] static extern bool EnumDisplaySettings(string dev, int mode, ref DEVMODE dm);
  // returns { width, height, currentHz, maxHzAtThisResolution }
  public static int[] Primary() {
    DEVMODE cur = new DEVMODE(); cur.dmSize = (short)Marshal.SizeOf(typeof(DEVMODE));
    if (!EnumDisplaySettings(null, -1, ref cur)) return new int[] { 0, 0, 0, 0 };
    int max = cur.dmDisplayFrequency;
    DEVMODE dm = new DEVMODE(); dm.dmSize = cur.dmSize;
    for (int i = 0; EnumDisplaySettings(null, i, ref dm); i++) {
      if (dm.dmPelsWidth == cur.dmPelsWidth && dm.dmPelsHeight == cur.dmPelsHeight && (dm.dmDisplayFlags & 2) == 0 && dm.dmDisplayFrequency > max)
        max = dm.dmDisplayFrequency;
    }
    return new int[] { cur.dmPelsWidth, cur.dmPelsHeight, cur.dmDisplayFrequency, max };
  }
}

public static class PowerThrottle {
  [StructLayout(LayoutKind.Sequential)]
  struct PPTS { public uint Version; public uint ControlMask; public uint StateMask; }
  [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
  [DllImport("kernel32.dll", SetLastError = true)] static extern bool SetProcessInformation(IntPtr h, int infoClass, ref PPTS info, int size);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  // off = true: never put this process in efficiency mode. off = false: hand control back to Windows.
  public static bool Set(int pid, bool off) {
    IntPtr h = OpenProcess(0x0200, false, pid);   // PROCESS_SET_INFORMATION
    if (h == IntPtr.Zero) return false;
    PPTS s = new PPTS(); s.Version = 1; s.ControlMask = off ? 1u : 0u; s.StateMask = 0;
    bool ok = false;
    try { ok = SetProcessInformation(h, 4, ref s, Marshal.SizeOf(typeof(PPTS))); } catch { }   // 4 = ProcessPowerThrottling
    CloseHandle(h);
    return ok;
  }
}

public static class FrameMon {
  static System.Diagnostics.Process proc;
  static readonly object sync = new object();
  static System.Collections.Generic.List<double> pending = new System.Collections.Generic.List<double>();
  static System.Collections.Generic.Dictionary<string, int> chains = new System.Collections.Generic.Dictionary<string, int>();
  static string topChain = null;
  static int colMs = -1, colChain = -1;
  public static string LastError = "";
  public static bool Start(string exe, string[] names) {
    Stop();
    lock (sync) { pending.Clear(); chains.Clear(); topChain = null; colMs = -1; colChain = -1; LastError = ""; }
    System.Text.StringBuilder a = new System.Text.StringBuilder("--output_stdout --no_console_stats --session_name ProjectOptM --stop_existing_session --terminate_on_proc_exit");
    foreach (string n in names) a.Append(" --process_name \"" + n + "\"");
    System.Diagnostics.ProcessStartInfo psi = new System.Diagnostics.ProcessStartInfo(exe, a.ToString());
    psi.UseShellExecute = false; psi.RedirectStandardOutput = true; psi.RedirectStandardError = true; psi.CreateNoWindow = true;
    try { proc = System.Diagnostics.Process.Start(psi); } catch (Exception e) { LastError = e.Message; return false; }
    System.Diagnostics.Process p = proc;
    System.Threading.Thread t1 = new System.Threading.Thread(() => { try { string l; while ((l = p.StandardOutput.ReadLine()) != null) Parse(l); } catch { } });
    t1.IsBackground = true; t1.Start();
    System.Threading.Thread t2 = new System.Threading.Thread(() => { try { string l; while ((l = p.StandardError.ReadLine()) != null) { if (l.Trim().Length > 0) LastError = l.Trim(); } } catch { } });
    t2.IsBackground = true; t2.Start();
    return true;
  }
  static void Parse(string line) {
    string[] f = line.Split(',');
    if (colMs < 0) {
      for (int i = 0; i < f.Length; i++) {
        string h = f[i].Trim();
        if (colMs < 0 && (h == "MsBetweenPresents" || h == "FrameTime" || h == "msBetweenPresents")) colMs = i;
        if (h == "SwapChainAddress") colChain = i;
      }
      if (colMs < 0 && line.StartsWith("Application")) LastError = "Unrecognized PresentMon output";
      return;
    }
    if (f.Length <= colMs || f[0] == "Application") return;
    double v;
    if (!double.TryParse(f[colMs], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out v)) return;
    if (v <= 0 || v > 2000) return;
    lock (sync) {
      if (colChain >= 0 && colChain < f.Length) {
        string c = f[colChain]; int n; chains.TryGetValue(c, out n); chains[c] = n + 1;
        if (topChain == null || chains[c] > chains[topChain]) topChain = c;
        if (c != topChain) return;
      }
      pending.Add(v);
    }
  }
  public static double[] Drain() { lock (sync) { double[] a = pending.ToArray(); pending.Clear(); return a; } }
  public static bool Running { get { try { return proc != null && !proc.HasExited; } catch { return false; } } }
  public static void Stop() { try { if (proc != null && !proc.HasExited) proc.Kill(); } catch { } proc = null; }
}

public static class FrameStats {
  // { fps (last 1s), 1% low (last 10s), avg frametime ms (last 1s) }
  public static double[] Stats(double[] ft) {
    int n = ft.Length; double sum = 0; int cnt = 0;
    for (int i = n - 1; i >= 0 && sum < 1000; i--) { sum += ft[i]; cnt++; }
    double fps = sum > 0 ? cnt * 1000.0 / sum : 0;
    double avg = cnt > 0 ? sum / cnt : 0;
    System.Collections.Generic.List<double> w = new System.Collections.Generic.List<double>();
    double s2 = 0;
    for (int i = n - 1; i >= 0 && s2 < 10000; i--) { s2 += ft[i]; w.Add(ft[i]); }
    double low = 0;
    if (w.Count >= 50) { w.Sort(); int idx = (int)Math.Ceiling(w.Count * 0.99) - 1; if (idx < 0) idx = 0; low = 1000.0 / w[idx]; }
    return new double[] { fps, low, avg };
  }
  // worst frametime per pixel column over the last windowMs (oldest on the left)
  public static double[] Columns(double[] ft, double windowMs, int cols) {
    double[] c = new double[cols];
    double t = 0;
    for (int i = ft.Length - 1; i >= 0; i--) {
      t += ft[i];
      if (t > windowMs) break;
      int col = cols - 1 - (int)(t / windowMs * cols);
      if (col < 0) col = 0;
      if (ft[i] > c[col]) c[col] = ft[i];
    }
    for (int i = 1; i < cols; i++) if (c[i] == 0) c[i] = c[i - 1];
    return c;
  }
  static long[] hist = new long[4001]; static double sSum; static long sN;
  public static void ResetSession() { Array.Clear(hist, 0, hist.Length); sSum = 0; sN = 0; }
  public static void AddSession(double[] ft) {
    foreach (double v in ft) { int b = (int)(v / 0.05); if (b > 4000) b = 4000; hist[b]++; sSum += v; sN++; }
  }
  // { average fps, 1% low } for the whole session
  public static double[] Session() {
    if (sN < 100 || sSum <= 0) return new double[] { 0, 0 };
    double avg = sN * 1000.0 / sSum;
    long need = (long)Math.Ceiling(sN * 0.01); long acc = 0; double ms = 0;
    for (int b = 4000; b >= 0; b--) { acc += hist[b]; if (acc >= need) { ms = (b + 0.5) * 0.05; break; } }
    return new double[] { avg, ms > 0 ? 1000.0 / ms : 0 };
  }
}

public static class CpuTopo {
  [DllImport("kernel32.dll", SetLastError = true)]
  static extern bool GetLogicalProcessorInformationEx(int rel, IntPtr buf, ref uint len);
  public static long AllMask, BestMask, OtherMask;
  public static string Kind = "Unknown";
  public static int L3Count;
  public static uint MaxL3MB, MinL3MB;
  public static void Detect() {
    uint len = 0;
    GetLogicalProcessorInformationEx(0xFFFF, IntPtr.Zero, ref len);
    if (len == 0) return;
    IntPtr buf = Marshal.AllocHGlobal((int)len);
    try {
      if (!GetLogicalProcessorInformationEx(0xFFFF, buf, ref len)) return;
      System.Collections.Generic.List<ulong> l3m = new System.Collections.Generic.List<ulong>();
      System.Collections.Generic.List<uint>  l3s = new System.Collections.Generic.List<uint>();
      System.Collections.Generic.List<ulong> cm  = new System.Collections.Generic.List<ulong>();
      System.Collections.Generic.List<byte>  ce  = new System.Collections.Generic.List<byte>();
      ulong all = 0; byte maxEff = 0;
      long off = 0;
      while (off < len) {
        IntPtr p = new IntPtr(buf.ToInt64() + off);
        int rel  = Marshal.ReadInt32(p, 0);
        int size = Marshal.ReadInt32(p, 4);
        if (size <= 0) break;
        if (rel == 0) {                       // RelationProcessorCore
          byte eff = Marshal.ReadByte(p, 9);
          ulong m  = (ulong)Marshal.ReadInt64(p, 32);
          short g  = Marshal.ReadInt16(p, 40);
          if (g == 0) { all |= m; cm.Add(m); ce.Add(eff); if (eff > maxEff) maxEff = eff; }
        } else if (rel == 2) {                // RelationCache
          byte level = Marshal.ReadByte(p, 8);
          uint csize = (uint)Marshal.ReadInt32(p, 12);
          int type   = Marshal.ReadInt32(p, 16);
          ulong m    = (ulong)Marshal.ReadInt64(p, 40);
          short g    = Marshal.ReadInt16(p, 48);
          if (level == 3 && type == 0 && g == 0) { l3m.Add(m); l3s.Add(csize); }
        }
        off += size;
      }
      AllMask = unchecked((long)all); BestMask = AllMask; OtherMask = 0;
      L3Count = l3m.Count;
      int big = -1; uint maxS = 0, minS = uint.MaxValue;
      for (int i = 0; i < l3s.Count; i++) {
        if (l3s[i] > maxS) { maxS = l3s[i]; big = i; }
        if (l3s[i] < minS) minS = l3s[i];
      }
      MaxL3MB = maxS / 1048576; MinL3MB = (l3s.Count > 0) ? minS / 1048576 : 0;
      if (maxEff > 0) {
        ulong pm = 0;
        for (int i = 0; i < cm.Count; i++) if (ce[i] == maxEff) pm |= cm[i];
        BestMask = unchecked((long)pm); OtherMask = unchecked((long)(all & ~pm));
        Kind = "Hybrid";
      } else if (l3m.Count >= 2 && maxS > minS) {
        BestMask = unchecked((long)(l3m[big] & all)); OtherMask = unchecked((long)(all & ~l3m[big]));
        Kind = "DualX3D";
      } else if (l3m.Count >= 2) {
        Kind = "DualCCD";
      } else if (l3m.Count == 1 && maxS >= 64u * 1048576) {
        Kind = "SingleX3D";
      } else {
        Kind = "Single";
      }
    } finally { Marshal.FreeHGlobal(buf); }
  }
}
'@
}

# Own taskbar identity (must happen before any window is shown)
try { [GameOptMem]::SetAppId('ProjectOptM.Optimizer') } catch {}

# ======================= System detection =======================
function Format-Mask([int64]$mask) {
  $idx = @(); for ($i = 0; $i -lt 64; $i++) { if (($mask -shr $i) -band 1) { $idx += $i } }
  if ($idx.Count -eq 0) { return 'none' }
  $out = @(); $s = $idx[0]; $prev = $idx[0]
  foreach ($n in (@($idx | Select-Object -Skip 1) + @(-99))) {
    if ($n -ne $prev + 1) { $out += $(if ($s -eq $prev) { "$s" } else { "$s-$prev" }); $s = $n }
    $prev = $n
  }
  return ($out -join ', ')
}

$Sys = [ordered]@{}

# CPU
$cpu = Get-CimInstance Win32_Processor
$cpuName  = (@($cpu)[0].Name).Trim()
$cores    = ($cpu | Measure-Object NumberOfCores -Sum).Sum
$logical  = [Environment]::ProcessorCount
try { [CpuTopo]::Detect() } catch {}
$topo = [CpuTopo]::Kind
$script:CanPin = ($logical -le 64) -and ([CpuTopo]::BestMask -ne [CpuTopo]::AllMask) -and ([CpuTopo]::BestMask -ne 0)
switch ($topo) {
  'DualX3D'   { $topoText = "Dual-CCD, 3D V-Cache ($([CpuTopo]::MaxL3MB)MB + $([CpuTopo]::MinL3MB)MB L3)"; $script:BestLabel = 'V-Cache cores'; $script:OtherLabel = 'frequency cores' }
  'Hybrid'    { $topoText = 'Hybrid (P-cores + E-cores)'; $script:BestLabel = 'P-cores'; $script:OtherLabel = 'E-cores' }
  'DualCCD'   { $topoText = "Dual-CCD ($([CpuTopo]::MaxL3MB)MB L3 each)"; $script:BestLabel = 'All cores'; $script:OtherLabel = 'All cores' }
  'SingleX3D' { $topoText = "3D V-Cache ($([CpuTopo]::MaxL3MB)MB L3)"; $script:BestLabel = 'All cores'; $script:OtherLabel = 'All cores' }
  default     { $topoText = 'Single cluster'; $script:BestLabel = 'All cores'; $script:OtherLabel = 'All cores' }
}
if (-not $script:CanPin) { $script:BestLabel = 'All cores'; $script:OtherLabel = 'All cores' }
$Sys.CPU = "$cpuName  |  $cores cores / $logical threads  |  $topoText"

# GPU (dedicated VRAM from the driver registry - WMI caps it at 4GB)
$vram = @{}
Get-ChildItem 'HKLM:\SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}' -ErrorAction SilentlyContinue |
  Where-Object { $_.PSChildName -match '^\d{4}$' } | ForEach-Object {
    $k = Get-ItemProperty -LiteralPath $_.PSPath -ErrorAction SilentlyContinue
    if ($k -and $k.DriverDesc) {
      $q = $k.'HardwareInformation.qwMemorySize'
      if ($q -is [byte[]]) { $q = [BitConverter]::ToUInt64($q, 0) }
      if ($q) { $vram[$k.DriverDesc] = [uint64]$q }
    }
  }
$gpus = @(Get-CimInstance Win32_VideoController | Where-Object { $_.Name -notmatch 'Basic Display|Basic Render|Remote|Virtual|Parsec|Meta |Oculus|Citrix|VNC|Hyper-V|Mirror' })
$gpuList = foreach ($g in $gpus) {
  $bytes = if ($vram.ContainsKey($g.Name)) { $vram[$g.Name] } else { [uint64]$g.AdapterRAM }
  [pscustomobject]@{ Name = $g.Name.Trim(); VRAM = $bytes; Obj = $g }
}
$gpuList = @($gpuList | Sort-Object VRAM -Descending)
if ($gpuList.Count -gt 0) {
  $main = $gpuList[0]
  $vramGB = [math]::Round($main.VRAM / 1GB)
  $Sys.GPU = "$($main.Name)  |  $vramGB GB"
  if ($gpuList.Count -gt 1) { $Sys.GPU += "  (+ $($gpuList[1].Name))" }
} else { $Sys.GPU = 'Not detected' }

# Hardware makers
$cpuMaker = "$(@($cpu)[0].Manufacturer)"
$cpuVendor = if ($cpuMaker -match 'AMD') { 'AMD' } elseif ($cpuMaker -match 'Intel') { 'Intel' } else { 'Other' }
function Get-GpuVendor($g) {
  $id = "$($g.Obj.PNPDeviceID)"
  if ($id -match 'VEN_10DE' -or $g.Name -match 'NVIDIA|GeForce') { return 'NVIDIA' }
  if ($id -match 'VEN_1002' -or $g.Name -match 'AMD|Radeon') { return 'AMD' }
  if ($id -match 'VEN_8086' -or $g.Name -match 'Intel') { return 'Intel' }
  return 'Other'
}
$gpuVendor  = if ($gpuList.Count -gt 0) { Get-GpuVendor $gpuList[0] } else { 'Other' }
$gpuVendors = @($gpuList | ForEach-Object { Get-GpuVendor $_ } | Select-Object -Unique)
$VendorApps = @{
  AMD    = @('RadeonSoftware', 'AMDRSServ', 'AMDRSSrcExt', 'amdow', 'cncmd')
  NVIDIA = @('NVIDIA app', 'NVIDIA Overlay', 'NVIDIA Share', 'NVIDIA Web Helper')
  Intel  = @('ArcControl', 'IntelGraphicsSoftware', 'IGCC')
}
$VendorColors = @{ AMD = '#ED1C24'; NVIDIA = '#76B900'; Intel = '#0071C5'; Other = '#8A90A0' }

# RAM
$mem = @(Get-CimInstance Win32_PhysicalMemory)
$ramGB  = [math]::Round(($mem | Measure-Object Capacity -Sum).Sum / 1GB)
$ramMTs = ($mem | Measure-Object ConfiguredClockSpeed -Maximum).Maximum
$ramType = switch ([int]$mem[0].SMBIOSMemoryType) { 34 { 'DDR5' } 26 { 'DDR4' } 24 { 'DDR3' } default { if ($ramMTs -ge 4800) { 'DDR5' } else { 'DDR4' } } }
$sizes = @($mem | ForEach-Object { [math]::Round($_.Capacity / 1GB) })
$layout = if (@($sizes | Select-Object -Unique).Count -eq 1) { "$($sizes.Count) x $($sizes[0]) GB" } else { ($sizes -join ' + ') + ' GB' }
$Sys.RAM = "$ramGB GB $ramType ($layout) @ $ramMTs MT/s"

# Display
$dispInfo = try { [DisplayInfo]::Primary() } catch { @(0, 0, 0, 0) }
if ($dispInfo[0] -gt 0) {
  $Sys.Display = "$($dispInfo[0]) x $($dispInfo[1]) @ $($dispInfo[2]) Hz"
  if ($dispInfo[3] -gt $dispInfo[2]) { $Sys.Display += " (supports $($dispInfo[3]) Hz)" }
} else {
  $disp = $gpus | Where-Object { $_.CurrentHorizontalResolution } | Select-Object -First 1
  $Sys.Display = if ($disp) { "$($disp.CurrentHorizontalResolution) x $($disp.CurrentVerticalResolution) @ $($disp.CurrentRefreshRate) Hz" } else { 'Not detected' }
}

# OS
$os = Get-CimInstance Win32_OperatingSystem
$ver = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' -ErrorAction SilentlyContinue).DisplayVersion
$Sys.OS = ($os.Caption -replace '^Microsoft ', '') + $(if ($ver) { " $ver" } else { '' }) + " (build $($os.BuildNumber))"

# --- Health checks (re-run whenever the window regains focus) ---
function New-Check($state, $text, $tip, $fix = $null, $fixLabel = 'Click to fix.') {
  [pscustomobject]@{ State = $state; Text = $text; Tip = $tip; Fix = $fix; FixLabel = $fixLabel }
}
function Open-Link($target) {
  # opened through Explorer so it runs as you, not as admin
  Start-Process -FilePath "$env:WINDIR\explorer.exe" -ArgumentList "`"$target`""
}

function Get-Checks {
  $list = @()

  # RAM speed
  $okSpeed = if ($ramType -eq 'DDR5') { 5600 } else { 3000 }
  if ($ramMTs -ge $okSpeed) { $list += New-Check 'ok' "RAM profile on ($ramMTs MT/s)" 'EXPO/XMP is active.' }
  else { $list += New-Check 'warn' "RAM at $ramMTs MT/s - EXPO/XMP may be off" 'Enable EXPO (AMD) or XMP (Intel) in your BIOS to run your RAM at its rated speed. This one has to be done in the BIOS.' }

  if ($ramGB -ge 48)     { $list += New-Check 'info' "$ramGB GB RAM - cleanup not needed" 'With this much RAM, standby cleanup is skipped for all games.' }
  elseif ($ramGB -le 16) { $list += New-Check 'info' "$ramGB GB RAM - extra cleanup on" 'Standby RAM is cleared twice as often in heavy games.' }
  else                   { $list += New-Check 'info' "$ramGB GB RAM - cleanup for heavy games" 'Standby RAM is cleared on a timer in games that need it.' }

  # Refresh rate
  $d = try { [DisplayInfo]::Primary() } catch { @(0, 0, 0, 0) }
  if ($d[2] -gt 0) {
    if ($d[3] -gt $d[2]) {
      $list += New-Check 'warn' "Display at $($d[2]) Hz (supports $($d[3]) Hz)" "Your monitor can do $($d[3]) Hz but Windows is running it at $($d[2]) Hz. Pick the highest rate under 'Choose a refresh rate'." { Open-Link 'ms-settings:display-advanced' } 'Click to open display settings.'
    } else { $list += New-Check 'ok' "Display at max refresh ($($d[2]) Hz)" 'Your main display is running at its highest refresh rate.' }
  }

  # Power plan
  $scheme = (powercfg /getactivescheme) -join ' '
  $planName = if ($scheme -match '\(([^)]+)\)') { $Matches[1] } else { 'Unknown' }
  if ($topo -eq 'DualX3D') {
    if ($scheme -match '381b4222-f694-41f0-9685-ff5bb260df2e') { $list += New-Check 'ok' 'Power plan: Balanced' 'Correct for X3D - lets the V-Cache core parking work.' }
    else { $list += New-Check 'warn' "Power plan: $planName" 'Dual-CCD X3D chips work best on Balanced. Other plans can stop the V-Cache core parking from working.' { powercfg /setactive SCHEME_BALANCED | Out-Null } 'Click to switch to Balanced.' }
    $svc = Get-Service -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -like '*V-Cache*' -or $_.Name -like 'amd3dv*' }
    if ($svc) { $list += New-Check 'ok' 'AMD V-Cache driver installed' 'The AMD 3D V-Cache Performance Optimizer is present.' }
    else { $list += New-Check 'warn' 'AMD V-Cache driver not found' 'Install the latest AMD chipset driver - it includes the 3D V-Cache Performance Optimizer.' { Open-Link 'https://www.amd.com/en/support/download/drivers.html' } 'Click to open AMD drivers.' }
    $gb = try { Get-AppxPackage -Name 'Microsoft.XboxGamingOverlay' -ErrorAction Stop } catch { 'unknown' }
    if (-not $gb) { $list += New-Check 'warn' 'Xbox Game Bar missing' "AMD's V-Cache driver uses Game Bar to tell when a game is running. Without it, games may land on the wrong cores." { Open-Link 'ms-windows-store://pdp/?ProductId=9NZKPSTSNW4P' } 'Click to open it in the Store.' }
  } else {
    $pp = switch ($script:Settings.PowerPlan) { 'off' { 'Not changed while gaming.' } default { 'Switches to High performance while a game runs, then back.' } }
    $list += New-Check 'info' "Power plan: $planName" $pp
  }

  # Game Mode
  $gm = (Get-ItemProperty 'HKCU:\Software\Microsoft\GameBar' -ErrorAction SilentlyContinue).AutoGameModeEnabled
  if ($gm -eq 0) {
    $list += New-Check 'warn' 'Game Mode off' 'Game Mode stops Windows Update from installing drivers mid-game and gives games scheduling priority.' {
      [Microsoft.Win32.Registry]::SetValue('HKEY_CURRENT_USER\Software\Microsoft\GameBar', 'AutoGameModeEnabled', 1, 'DWord')
      [Microsoft.Win32.Registry]::SetValue('HKEY_CURRENT_USER\Software\Microsoft\GameBar', 'AllowAutoGameMode', 1, 'DWord')
    } 'Click to turn it on.'
  } else { $list += New-Check 'ok' 'Game Mode on' 'Windows Game Mode is enabled.' }

  # Background recording
  $hc = (Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\GameDVR' -ErrorAction SilentlyContinue).HistoricalCaptureEnabled
  if ($hc -eq 1) {
    $list += New-Check 'warn' 'Background recording on' "Game Bar's 'Record what happened' constantly records gameplay, which costs FPS. You can still record clips manually with it off." {
      [Microsoft.Win32.Registry]::SetValue('HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\GameDVR', 'HistoricalCaptureEnabled', 0, 'DWord')
    } 'Click to turn it off.'
  } else { $list += New-Check 'ok' 'Background recording off' "Game Bar isn't constantly recording in the background." }

  # Memory Integrity (security trade-off - info only)
  $dg = Get-CimInstance -Namespace 'root\Microsoft\Windows\DeviceGuard' -ClassName Win32_DeviceGuard -ErrorAction SilentlyContinue
  if ($dg -and (@($dg.SecurityServicesRunning) -contains 2)) {
    $list += New-Check 'info' 'Memory Integrity on' "Memory Integrity (Core isolation) can cost a few percent FPS in some games, but it's a real security feature. Turning it off is a trade-off - your call." { Open-Link 'windowsdefender://coreisolation' } 'Click to open Core isolation settings.'
  }

  # Multiple GPUs
  if ($gpuList.Count -ge 2 -and $script:Settings.ForceGpu) {
    $list += New-Check 'info' "Games locked to $($gpuList[0].Name)" "You also have $($gpuList[1].Name) active. Each game is set to always use your main GPU (Settings > Display > Graphics)."
  }
  # ----- Maker-specific checks -----
  # GPU driver age (all makers)
  if ($gpuList.Count -gt 0 -and $gpuList[0].Obj.DriverDate) {
    $drv = [datetime]$gpuList[0].Obj.DriverDate
    $age = ((Get-Date) - $drv).Days
    $drvLink = switch ($gpuVendor) {
      'NVIDIA' { 'https://www.nvidia.com/en-us/drivers/' }
      'AMD'    { 'https://www.amd.com/en/support/download/drivers.html' }
      'Intel'  { 'https://www.intel.com/content/www/us/en/support/detect.html' }
      default  { $null }
    }
    if ($age -gt 180) {
      $fixB = if ($drvLink) { [scriptblock]::Create("Open-Link '$drvLink'") } else { $null }
      $list += New-Check 'warn' "$gpuVendor driver is $([math]::Floor($age / 30)) months old" "Your graphics driver is from $($drv.ToString('MMMM yyyy')). New drivers often include fixes and performance improvements for recent games." $fixB 'Click to open the driver download page.'
    } else {
      $list += New-Check 'ok' "$gpuVendor driver recent ($($drv.ToString('MMM yyyy')))" 'Your graphics driver is less than 6 months old.'
    }
  }
  # NVIDIA RTX 40/50: DLSS Frame Generation needs hardware-accelerated GPU scheduling
  if ($gpuVendor -eq 'NVIDIA' -and $gpuList[0].Name -match 'RTX\s*(40|50)\d\d') {
    $hs = (Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers' -ErrorAction SilentlyContinue).HwSchMode
    if ($hs -eq 1) {
      $list += New-Check 'warn' 'GPU scheduling off' 'NVIDIA DLSS Frame Generation needs Hardware-accelerated GPU scheduling. Turning it on takes effect after a restart.' {
        [Microsoft.Win32.Registry]::SetValue('HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers', 'HwSchMode', 2, 'DWord')
        Write-Log '  Restart your PC to finish turning on GPU scheduling'
      } 'Click to turn it on (restart needed).'
    } else { $list += New-Check 'ok' 'GPU scheduling on' 'Needed for DLSS Frame Generation on RTX 40 and 50 series cards.' }
  }
  # Intel 13th/14th gen desktop: microcode fix for the instability issue
  if ($cpuVendor -eq 'Intel' -and $cpuName -match '\bi[579]-1[34]\d{3}(KS|KF|K|F|T)?\b') {
    $ur = (Get-ItemProperty 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor\0' -ErrorAction SilentlyContinue).'Update Revision'
    if ($ur -is [byte[]] -and $ur.Length -ge 8) {
      $rev = [BitConverter]::ToUInt32($ur, 4)
      if ($rev -eq 0) { $rev = [BitConverter]::ToUInt32($ur, 0) }
      $hex = '0x{0:X}' -f $rev
      if ($rev -lt 0x12B) {
        $list += New-Check 'warn' "Intel microcode $hex - update your BIOS" "13th and 14th gen Core i5/i7/i9 desktop chips need microcode 0x12B or newer to prevent the instability and degradation problem Intel confirmed in 2024. Update your motherboard BIOS to get it - this can't be fixed from Windows."
      } else { $list += New-Check 'ok' "Intel microcode $hex" "Includes Intel's fix for the 13th/14th gen instability problem." }
    }
  }
  # Intel Arc: Resizable BAR
  if ($gpuVendor -eq 'Intel' -and $gpuList[0].Name -match 'Arc') {
    $list += New-Check 'info' 'Intel Arc: keep Resizable BAR on' 'Arc cards lose a lot of performance without Resizable BAR. Make sure it and Above 4G Decoding are turned on in your BIOS.'
  }
  return $list
}
$script:Checks = Get-Checks
$script:LastCheck = Get-Date

# RAM cleanup interval, adjusted for installed RAM
function Get-Purge($p) {
  if ($p.PurgeMins -le 0 -or $ramGB -ge 48) { return 0 }
  if ($ramGB -le 16) { return [math]::Max(5, [int]($p.PurgeMins / 2)) }
  return $p.PurgeMins
}

# --- Saved settings (launch shortcuts, auto on/off) ---
$ConfigPath = Join-Path $env:APPDATA 'ProjectOptM\settings.json'
$oldConfig  = Join-Path $env:APPDATA 'GameOptimizer\settings.json'
if (-not (Test-Path -LiteralPath $ConfigPath) -and (Test-Path -LiteralPath $oldConfig)) {
  try { New-Item -ItemType Directory -Force -Path (Split-Path $ConfigPath) | Out-Null; Copy-Item -LiteralPath $oldConfig -Destination $ConfigPath } catch {}
}
$script:Paths  = @{}
$script:AutoOn = $true
$script:IfeoManaged = @()
$script:RestorePlan = $null
$DefaultTheme = @{ Accent = '#4F8BFF'; Bg = 'Dark'; Corners = 'Rounded' }
$script:Theme = $DefaultTheme.Clone()
$script:FpsOn = $true
$script:AutoUpdate = $true
$script:PendingSvcs = @()
$script:OptImportMark = $null
if (Test-Path -LiteralPath $ConfigPath) {
  try {
    $c = Get-Content -LiteralPath $ConfigPath -Raw | ConvertFrom-Json
    if ($c.Paths) { $c.Paths.PSObject.Properties | ForEach-Object { $script:Paths[$_.Name] = $_.Value } }
    if ($null -ne $c.Auto) { $script:AutoOn = [bool]$c.Auto }
    if ($c.Ifeo) { $script:IfeoManaged = @($c.Ifeo) }
    if ($c.RestorePlan) { $script:RestorePlan = $c.RestorePlan }
    if ($c.Theme) { foreach ($k in @('Accent', 'Bg', 'Corners')) { if ($c.Theme.$k) { $script:Theme[$k] = $c.Theme.$k } } }
    if ($null -ne $c.FpsOn) { $script:FpsOn = [bool]$c.FpsOn }
    if ($null -ne $c.AutoUpdate) { $script:AutoUpdate = [bool]$c.AutoUpdate }
    if ($c.PausedSvcs) { $script:PendingSvcs = @($c.PausedSvcs) }
    if ($c.OptimizerImport) { $script:OptImportMark = "$($c.OptimizerImport)" }
  } catch {}
}
function Save-Config {
  try {
    New-Item -ItemType Directory -Force -Path (Split-Path $ConfigPath) | Out-Null
    @{ Paths = $script:Paths; Auto = $script:AutoOn; Ifeo = @($script:IfeoManaged); RestorePlan = $script:RestorePlan; Theme = $script:Theme; FpsOn = $script:FpsOn; AutoUpdate = $script:AutoUpdate; PausedSvcs = @($script:StoppedSvcs); OptimizerImport = $script:OptImportMark } | ConvertTo-Json | Set-Content -LiteralPath $ConfigPath -Encoding UTF8
  } catch {}
}

# --- State ---
$script:Saved = @{}      # PID -> original priority
$script:Tuned = @{}      # game PIDs already tuned
$script:Active = $null   # profile currently applied
$script:LastPurge = Get-Date
$script:Rows = @{}
$script:TrayTipShown = $false
$script:StoppedSvcs = @()
$script:EcoPids = @()
$HistoryPath = Join-Path $env:APPDATA 'ProjectOptM\history.csv'
$script:SessionStart = $null
$script:SessionCleanups = 0
$UpdateServices = @('UsoSvc', 'wuauserv', 'DoSvc')
$ProtectedNames = @('explorer', 'dwm', 'csrss', 'winlogon', 'lsass', 'services', 'svchost', 'System', 'smss', 'wininit', 'powershell', 'conhost', 'audiodg',
                    'vgc', 'vgtray', 'BEService', 'BEService_x64', 'EasyAntiCheat', 'EasyAntiCheat_EOS', 'FACEIT', 'faceitservice', 'OVRServer_x64')
$LauncherGroups = @{
  steam     = @('steam', 'steamwebhelper', 'steamservice')
  epic      = @('EpicGamesLauncher', 'EpicWebHelper')
  riot      = @('RiotClientServices', 'RiotClientUx', 'RiotClientUxRender')
  battlenet = @('Battle.net', 'Agent')
  ea        = @('EADesktop', 'EABackgroundService')
  oculus    = @('OculusClient', 'OVRRedir', 'OVRServiceLauncher')
  xbox      = @('XboxPcApp', 'XboxPcAppFT', 'XboxApp', 'XboxGameOverlay')
}
$AllLauncherNames = @($LauncherGroups.Values | ForEach-Object { $_ })
function Expand-AppNames($list) {
  $out = @()
  foreach ($n in @($list)) { if (-not $n) { continue }; if ($LauncherGroups.ContainsKey($n)) { $out += $LauncherGroups[$n] } else { $out += $n } }
  return @($out | Select-Object -Unique)
}
$script:ReopenPaths = @()
$script:Playtime = @{}

$bc = New-Object System.Windows.Media.BrushConverter
$script:Brush = @{
  Green = $bc.ConvertFromString('#3DDC84'); Amber = $bc.ConvertFromString('#F5B942')
  Idle  = $bc.ConvertFromString('#3A3F4B'); Gray  = $bc.ConvertFromString('#6B7180')
}

# ======================= UI =======================
$windowXaml = @'
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Project OptM" Width="1240" Height="840" MinWidth="980" MinHeight="620"
        WindowStartupLocation="CenterScreen" Background="{DynamicResource Bg}" FontFamily="Segoe UI">
  <Window.Resources>
    <SolidColorBrush x:Key="Bg" Color="#0F1115"/>
    <SolidColorBrush x:Key="Card" Color="#181B22"/>
    <SolidColorBrush x:Key="Card2" Color="#12151B"/>
    <SolidColorBrush x:Key="Btn" Color="#262B36"/>
    <SolidColorBrush x:Key="Chip" Color="#222631"/>
    <SolidColorBrush x:Key="Line" Color="#2A2F3A"/>
    <SolidColorBrush x:Key="Text" Color="#E8EAF0"/>
    <SolidColorBrush x:Key="Sub" Color="#8A90A0"/>
    <SolidColorBrush x:Key="Dim" Color="#6B7180"/>
    <SolidColorBrush x:Key="Faint" Color="#5A6070"/>
    <SolidColorBrush x:Key="Accent" Color="#4F8BFF"/>
    <SolidColorBrush x:Key="AccentSoft" Color="#334F8BFF"/>
    <SolidColorBrush x:Key="AccentText" Color="#FFFFFF"/>
    <CornerRadius x:Key="CardRadius">10</CornerRadius>
    <CornerRadius x:Key="BtnRadius">6</CornerRadius>
    <CornerRadius x:Key="ChipRadius">11</CornerRadius>
    <Style TargetType="Button">
      <Setter Property="Foreground" Value="{DynamicResource Text}"/>
      <Setter Property="Background" Value="{DynamicResource Btn}"/>
      <Setter Property="Padding" Value="14,7"/>
      <Setter Property="FontWeight" Value="SemiBold"/>
      <Setter Property="FontSize" Value="12"/>
      <Setter Property="Cursor" Value="Hand"/>
      <Setter Property="Template">
        <Setter.Value>
          <ControlTemplate TargetType="Button">
            <Border x:Name="b" Background="{TemplateBinding Background}" CornerRadius="{DynamicResource BtnRadius}" Padding="{TemplateBinding Padding}">
              <ContentPresenter HorizontalAlignment="Center" VerticalAlignment="Center"/>
            </Border>
            <ControlTemplate.Triggers>
              <Trigger Property="IsMouseOver" Value="True"><Setter TargetName="b" Property="Opacity" Value="0.85"/></Trigger>
              <Trigger Property="IsPressed" Value="True"><Setter TargetName="b" Property="Opacity" Value="0.7"/></Trigger>
              <Trigger Property="IsEnabled" Value="False"><Setter TargetName="b" Property="Opacity" Value="0.45"/></Trigger>
            </ControlTemplate.Triggers>
          </ControlTemplate>
        </Setter.Value>
      </Setter>
    </Style>
    <Style x:Key="NavButton" TargetType="Button">
      <Setter Property="Foreground" Value="{DynamicResource Sub}"/>
      <Setter Property="Background" Value="Transparent"/>
      <Setter Property="FontSize" Value="13"/>
      <Setter Property="FontWeight" Value="SemiBold"/>
      <Setter Property="Cursor" Value="Hand"/>
      <Setter Property="Height" Value="42"/>
      <Setter Property="Margin" Value="0,0,0,4"/>
      <Setter Property="Template">
        <Setter.Value>
          <ControlTemplate TargetType="Button">
            <Border x:Name="b" Background="{TemplateBinding Background}" CornerRadius="{DynamicResource BtnRadius}" Padding="10,0">
              <ContentPresenter HorizontalAlignment="Left" VerticalAlignment="Center"/>
            </Border>
            <ControlTemplate.Triggers>
              <Trigger Property="IsMouseOver" Value="True"><Setter TargetName="b" Property="Opacity" Value="0.8"/></Trigger>
            </ControlTemplate.Triggers>
          </ControlTemplate>
        </Setter.Value>
      </Setter>
    </Style>
  </Window.Resources>

  <Grid Margin="16">
    <Grid.ColumnDefinitions>
      <ColumnDefinition Width="220"/>
      <ColumnDefinition Width="*"/>
    </Grid.ColumnDefinitions>

    <!-- ===== Sidebar ===== -->
    <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="14,16" Margin="0,0,16,0">
      <DockPanel>
        <StackPanel DockPanel.Dock="Top" Orientation="Horizontal" Margin="4,0,0,22">
          <Grid Width="38" Height="38" Margin="0,0,12,0">
            <Border CornerRadius="{DynamicResource CardRadius}" BorderBrush="{DynamicResource Accent}" BorderThickness="1" Opacity="0.55"/>
            <Border CornerRadius="{DynamicResource CardRadius}" Margin="1">
              <Border.Background>
                <LinearGradientBrush StartPoint="0,0" EndPoint="0,1">
                  <GradientStop Color="#14171E" Offset="0"/>
                  <GradientStop Color="#030406" Offset="1"/>
                </LinearGradientBrush>
              </Border.Background>
            </Border>
            <Viewbox Width="38" Height="38">
              <Canvas Width="40" Height="40">
                <Ellipse Canvas.Left="6.2" Canvas.Top="6.2" Width="27.6" Height="27.6" Stroke="{DynamicResource Accent}" StrokeThickness="4.6"/>
                <Line X1="20" Y1="20" X2="24.6" Y2="15.4" Stroke="#FFFFFF" StrokeThickness="2.6" StrokeStartLineCap="Round" StrokeEndLineCap="Round"/>
                <Ellipse Canvas.Left="17.9" Canvas.Top="17.9" Width="4.2" Height="4.2" Fill="#FFFFFF"/>
              </Canvas>
            </Viewbox>
          </Grid>
          <StackPanel VerticalAlignment="Center">
            <TextBlock FontSize="17" FontWeight="Bold">
              <Run Text="PROJECT " Foreground="{DynamicResource Text}"/><Run Text="OPTM" Foreground="{DynamicResource Accent}"/>
            </TextBlock>
            <TextBlock x:Name="SubTitle" Foreground="{DynamicResource Dim}" FontSize="11"/>
          </StackPanel>
        </StackPanel>

        <StackPanel DockPanel.Dock="Bottom">
          <Button x:Name="AutoBtn" HorizontalAlignment="Stretch" Padding="14,9" ToolTip="Turn automatic optimizing on or off"/>
          <TextBlock Text="Ctrl+Alt+End = panic" Foreground="{DynamicResource Faint}" FontSize="10" HorizontalAlignment="Center" Margin="0,8,0,0"/>
        </StackPanel>

        <StackPanel x:Name="NavPanel"/>
      </DockPanel>
    </Border>

    <!-- ===== Content ===== -->
    <Grid Grid.Column="1">
      <Grid.RowDefinitions>
        <RowDefinition Height="Auto"/>
        <RowDefinition Height="Auto"/>
        <RowDefinition Height="*"/>
      </Grid.RowDefinitions>

      <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
        <Grid>
          <Grid.ColumnDefinitions><ColumnDefinition Width="Auto"/><ColumnDefinition Width="*"/><ColumnDefinition Width="Auto"/></Grid.ColumnDefinitions>
          <Ellipse x:Name="StatusDot" Width="14" Height="14" Margin="0,0,14,0" VerticalAlignment="Center"/>
          <StackPanel Grid.Column="1" VerticalAlignment="Center">
            <TextBlock x:Name="StatusText" Foreground="{DynamicResource Text}" FontSize="16" FontWeight="SemiBold" TextTrimming="CharacterEllipsis"/>
            <TextBlock x:Name="StatusSub" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,2,0,0" TextTrimming="CharacterEllipsis"/>
          </StackPanel>
          <Button x:Name="UpdateBtn" Grid.Column="2" Visibility="Collapsed" Margin="12,0,0,0" Background="{DynamicResource Accent}" Foreground="{DynamicResource AccentText}"
                  ToolTip="A new version is available - click to see what's new"/>
        </Grid>
      </Border>

      <TextBlock x:Name="PageTitle" Grid.Row="1" Foreground="{DynamicResource Text}" FontSize="22" FontWeight="Bold" Margin="4,0,0,12"/>

      <Grid Grid.Row="2">

        <!-- ===== HOME ===== -->
        <Grid x:Name="PageHome">
          <Grid.RowDefinitions><RowDefinition Height="Auto"/><RowDefinition Height="*"/></Grid.RowDefinitions>
          <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="16,14" Margin="0,0,0,14">
            <Grid>
              <Grid.ColumnDefinitions><ColumnDefinition Width="180"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
              <StackPanel VerticalAlignment="Center">
                <TextBlock Text="PERFORMANCE" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <StackPanel Orientation="Horizontal" Margin="0,2,0,0">
                  <TextBlock x:Name="FpsText" Text="--" Foreground="{DynamicResource Text}" FontSize="40" FontWeight="Bold"/>
                  <TextBlock Text="FPS" Foreground="{DynamicResource Accent}" FontSize="14" FontWeight="SemiBold" VerticalAlignment="Bottom" Margin="6,0,0,9"/>
                </StackPanel>
                <TextBlock x:Name="LowText" Text="1% low      --" Foreground="{DynamicResource Sub}" FontSize="12" FontFamily="Consolas"/>
                <TextBlock x:Name="FtText" Text="Frametime   --" Foreground="{DynamicResource Sub}" FontSize="12" FontFamily="Consolas" Margin="0,2,0,0"/>
                <Button x:Name="FpsBtn" Margin="0,10,0,0" HorizontalAlignment="Left" Padding="10,4" FontSize="11"/>
              </StackPanel>
              <Grid Grid.Column="1">
                <Border x:Name="ChartHost" Background="{DynamicResource Card2}" CornerRadius="{DynamicResource BtnRadius}" Height="170" ClipToBounds="True">
                  <Canvas x:Name="Chart"/>
                </Border>
                <TextBlock x:Name="ChartMsg" HorizontalAlignment="Center" VerticalAlignment="Center" TextAlignment="Center"
                           Foreground="{DynamicResource Dim}" FontSize="12" TextWrapping="Wrap" Margin="20,0"/>
              </Grid>
            </Grid>
          </Border>
          <Grid Grid.Row="1">
            <Grid.ColumnDefinitions><ColumnDefinition Width="*"/><ColumnDefinition Width="300"/></Grid.ColumnDefinitions>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="16,14" Margin="0,0,14,0">
              <DockPanel>
                <TextBlock DockPanel.Dock="Top" Text="RECENT SESSIONS" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold" Margin="0,0,0,10"/>
                <ScrollViewer VerticalScrollBarVisibility="Auto"><StackPanel x:Name="RecentPanel"/></ScrollViewer>
              </DockPanel>
            </Border>
            <StackPanel Grid.Column="1">
              <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="16,14" Margin="0,0,0,14">
                <StackPanel>
                  <TextBlock Text="SYSTEM HEALTH" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                  <StackPanel Orientation="Horizontal" Margin="0,10,0,0">
                    <Ellipse x:Name="HealthDot" Width="10" Height="10" Margin="0,0,10,0" VerticalAlignment="Center"/>
                    <TextBlock x:Name="HealthText" Foreground="{DynamicResource Text}" FontSize="14" FontWeight="SemiBold"/>
                  </StackPanel>
                  <Button x:Name="HealthBtn" Content="View checks" HorizontalAlignment="Left" Margin="0,12,0,0" Padding="10,5" FontSize="11"/>
                </StackPanel>
              </Border>
              <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="16,14">
                <StackPanel>
                  <TextBlock Text="MOST PLAYED" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold" Margin="0,0,0,10"/>
                  <StackPanel x:Name="TopPanel"/>
                </StackPanel>
              </Border>
            </StackPanel>
          </Grid>
        </Grid>

        <!-- ===== GAMES ===== -->
        <Grid x:Name="PageGames" Visibility="Collapsed">
          <Grid.RowDefinitions><RowDefinition Height="Auto"/><RowDefinition Height="*"/></Grid.RowDefinitions>
          <Grid Margin="2,0,10,12">
            <TextBlock Text="Launch from here or anywhere else - games are optimized automatically." Foreground="{DynamicResource Sub}" FontSize="12" VerticalAlignment="Center"/>
            <Button x:Name="EditBtn" Content="Edit profiles" HorizontalAlignment="Right"
                    ToolTip="Opens your game profiles in Notepad. Saving reloads them automatically."/>
          </Grid>
          <ScrollViewer Grid.Row="1" x:Name="GameScroll" VerticalScrollBarVisibility="Auto">
            <UniformGrid x:Name="GameList" Columns="2" VerticalAlignment="Top"/>
          </ScrollViewer>
        </Grid>

        <!-- ===== SYSTEM ===== -->
        <ScrollViewer x:Name="PageSystem" Visibility="Collapsed" VerticalScrollBarVisibility="Auto">
          <StackPanel MaxWidth="820" HorizontalAlignment="Left">
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
              <StackPanel>
                <Grid Margin="0,0,0,10">
                  <TextBlock Text="YOUR SYSTEM" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold" VerticalAlignment="Center"/>
                  <Button x:Name="CopyBtn" Content="Copy specs" HorizontalAlignment="Right" Padding="10,4" FontSize="11"/>
                </Grid>
                <Grid x:Name="SpecGrid">
                  <Grid.ColumnDefinitions><ColumnDefinition Width="80"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
                </Grid>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
              <StackPanel>
                <TextBlock Text="TUNED FOR YOUR HARDWARE" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <StackPanel x:Name="VendorPanel" Margin="0,12,0,0"/>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14">
              <StackPanel>
                <TextBlock Text="HEALTH CHECKS" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock Text="Outlined checks can be fixed with one click. Hover any check for details." Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,4,0,12"/>
                <WrapPanel x:Name="CheckPanel"/>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,14,0,0">
              <StackPanel>
                <TextBlock Text="TOOLS" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock Text="Clear the shader cache after a graphics driver update, or if a game starts stuttering or crashing more than usual. Games rebuild it on their next launch." Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,6,0,0" TextWrapping="Wrap"/>
                <Button x:Name="ShaderBtn" Content="Clear shader cache" HorizontalAlignment="Left" Margin="0,12,0,0"/>
              </StackPanel>
            </Border>
          </StackPanel>
        </ScrollViewer>

        <!-- ===== ACTIVITY ===== -->
        <Border x:Name="PageActivity" Visibility="Collapsed" Background="{DynamicResource Card2}" CornerRadius="{DynamicResource CardRadius}" Padding="16,12">
          <TextBox x:Name="LogBox" IsReadOnly="True" Background="Transparent" BorderThickness="0"
                   Foreground="{DynamicResource Sub}" FontFamily="Consolas" FontSize="12" TextWrapping="Wrap"
                   VerticalScrollBarVisibility="Auto"/>
        </Border>

        <!-- ===== SETTINGS ===== -->
        <ScrollViewer x:Name="PageSettings" Visibility="Collapsed" VerticalScrollBarVisibility="Auto">
          <StackPanel MaxWidth="680" HorizontalAlignment="Left">
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
              <StackPanel>
                <TextBlock Text="APPEARANCE" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock Text="Accent color" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,12,0,0"/>
                <WrapPanel x:Name="AccentPanel" Margin="0,8,0,0"/>
                <StackPanel Orientation="Horizontal" Margin="0,2,0,0">
                  <TextBox x:Name="HexBox" Background="{DynamicResource Card2}" Foreground="{DynamicResource Text}" CaretBrush="{DynamicResource Text}"
                           BorderBrush="{DynamicResource Line}" BorderThickness="1" Padding="8,6" FontFamily="Consolas" FontSize="12" Width="200"
                           VerticalContentAlignment="Center" ToolTip="Any color as #RRGGBB"/>
                  <Button x:Name="HexBtn" Content="Use" Margin="8,0,0,0"/>
                </StackPanel>
                <TextBlock Text="Background" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,16,0,0"/>
                <WrapPanel x:Name="BgPanel" Margin="0,8,0,0"/>
                <TextBlock Text="Corners" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,10,0,0"/>
                <WrapPanel x:Name="CornerPanel" Margin="0,8,0,0"/>
                <Button x:Name="ResetThemeBtn" Content="Reset to default" Margin="0,8,0,0" HorizontalAlignment="Left"/>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
              <StackPanel>
                <TextBlock Text="PROFILES AND DATA" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock Text="Game profiles, settings, play history and backups live in one folder." Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,6,0,10" TextWrapping="Wrap"/>
                <WrapPanel>
                  <Button x:Name="EditBtn2" Content="Edit profiles" Margin="0,0,8,6"/>
                  <Button x:Name="OpenDataBtn" Content="Open data folder" Margin="0,0,8,6"/>
                </WrapPanel>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14" Margin="0,0,0,14">
              <StackPanel>
                <TextBlock Text="UPDATES" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock x:Name="AboutVersion" Foreground="{DynamicResource Text}" FontSize="14" FontWeight="SemiBold" Margin="0,10,0,0"/>
                <TextBlock x:Name="AboutStatus" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,2,0,0" TextWrapping="Wrap"/>
                <WrapPanel Margin="0,12,0,0">
                  <Button x:Name="CheckUpdBtn" Content="Check for updates" Margin="0,0,8,6"/>
                  <Button x:Name="AutoUpdBtn" Margin="0,0,8,6" ToolTip="Check for new versions every few hours"/>
                </WrapPanel>
              </StackPanel>
            </Border>
            <Border Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="18,14">
              <StackPanel>
                <TextBlock Text="SHORTCUTS" Foreground="{DynamicResource Dim}" FontSize="11" FontWeight="Bold"/>
                <TextBlock Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,10,0,0" LineHeight="20">
                  <Run Text="Ctrl + 1 to 5" Foreground="{DynamicResource Text}"/><Run Text="      switch tabs"/><LineBreak/>
                  <Run Text="Ctrl + Alt + End" Foreground="{DynamicResource Text}"/><Run Text="   panic - undo everything instantly"/>
                </TextBlock>
              </StackPanel>
            </Border>
          </StackPanel>
        </ScrollViewer>

      </Grid>
    </Grid>
  </Grid>
</Window>
'@

$rowXaml = @'
<Border xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Background="{DynamicResource Card}" CornerRadius="{DynamicResource CardRadius}" Padding="16,14" Margin="0,0,10,10">
  <DockPanel>
    <Grid DockPanel.Dock="Bottom" Margin="0,12,0,0">
      <Grid.ColumnDefinitions><ColumnDefinition Width="Auto"/><ColumnDefinition Width="Auto"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
      <Button x:Name="SetBtn" Content="SET" Margin="0,0,8,0" ToolTip="Pick the game's desktop shortcut or .exe"/>
      <Button x:Name="InfoBtn" Grid.Column="1" Content="PREVIEW" Margin="0,0,8,0" ToolTip="See exactly what this profile will do on your PC - nothing is changed"/>
      <Button x:Name="PlayBtn" Grid.Column="2" Content="PLAY" Background="{DynamicResource Accent}" Foreground="{DynamicResource AccentText}"/>
    </Grid>
    <StackPanel>
      <Grid>
        <Grid.ColumnDefinitions><ColumnDefinition Width="Auto"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
        <Ellipse x:Name="Dot" Width="10" Height="10" Fill="#3A3F4B" VerticalAlignment="Center" Margin="0,0,10,0"/>
        <TextBlock x:Name="Title" Grid.Column="1" Foreground="{DynamicResource Text}" FontSize="15" FontWeight="SemiBold" TextTrimming="CharacterEllipsis"/>
      </Grid>
      <TextBlock x:Name="Sub" Foreground="{DynamicResource Sub}" FontSize="12" Margin="0,6,0,0" TextWrapping="Wrap"/>
      <TextBlock x:Name="PathText" Foreground="{DynamicResource Faint}" FontSize="11" Margin="0,4,0,0" TextTrimming="CharacterEllipsis"/>
    </StackPanel>
  </DockPanel>
</Border>
'@

$script:Window     = [System.Windows.Markup.XamlReader]::Parse($windowXaml)
$script:LogBox     = $script:Window.FindName('LogBox')
$script:StatusDot  = $script:Window.FindName('StatusDot')
$script:StatusText = $script:Window.FindName('StatusText')
$script:StatusSub  = $script:Window.FindName('StatusSub')
$script:AutoBtn    = $script:Window.FindName('AutoBtn')
$gameList          = $script:Window.FindName('GameList')
$wa = [System.Windows.SystemParameters]::WorkArea
$script:Window.Width  = [math]::Min(1240, $wa.Width - 20)
$script:Window.Height = [math]::Min(840,  $wa.Height - 20)
$script:Window.FindName('GameScroll').Add_SizeChanged({
  param($sender, $e)
  $cols = [math]::Floor($e.NewSize.Width / 320)
  $gameList.Columns = [math]::Max(1, [math]::Min(4, $cols))
})

# ---------- sidebar navigation ----------
$Pages = [ordered]@{
  'Home'     = @([char]0xE80F, 'PageHome')
  'Games'    = @([char]0xE7FC, 'PageGames')
  'System'   = @([char]0xE7F4, 'PageSystem')
  'Activity' = @([char]0xE81C, 'PageActivity')
  'Settings' = @([char]0xE713, 'PageSettings')
}
$script:Nav = @{}
$navPanel = $script:Window.FindName('NavPanel')
$iconFont = New-Object System.Windows.Media.FontFamily('Segoe Fluent Icons, Segoe MDL2 Assets')
foreach ($pageName in $Pages.Keys) {
  $b = New-Object System.Windows.Controls.Button
  $b.Style = $script:Window.Resources['NavButton']
  $b.Tag = $pageName
  $b.HorizontalAlignment = 'Stretch'
  $g = New-Object System.Windows.Controls.Grid
  foreach ($w in @('Auto', 'Auto', 'Star')) {
    $cd = New-Object System.Windows.Controls.ColumnDefinition
    if ($w -eq 'Star') { $cd.Width = New-Object System.Windows.GridLength(1, [System.Windows.GridUnitType]::Star) }
    else { $cd.Width = [System.Windows.GridLength]::Auto }
    [void]$g.ColumnDefinitions.Add($cd)
  }
  $bar = New-Object System.Windows.Shapes.Rectangle
  $bar.Width = 3; $bar.Height = 18; $bar.RadiusX = 1.5; $bar.RadiusY = 1.5
  $bar.Margin = New-Object System.Windows.Thickness(0,0,12,0); $bar.Visibility = 'Hidden'
  $bar.SetResourceReference([System.Windows.Shapes.Shape]::FillProperty, 'Accent')
  $ic = New-Object System.Windows.Controls.TextBlock
  $ic.Text = [string]$Pages[$pageName][0]; $ic.FontFamily = $iconFont; $ic.FontSize = 16; $ic.FontWeight = 'Normal'
  $ic.Margin = New-Object System.Windows.Thickness(0,0,12,0); $ic.VerticalAlignment = 'Center'
  $lb = New-Object System.Windows.Controls.TextBlock
  $lb.Text = $pageName; $lb.VerticalAlignment = 'Center'
  [System.Windows.Controls.Grid]::SetColumn($ic, 1); [System.Windows.Controls.Grid]::SetColumn($lb, 2)
  [void]$g.Children.Add($bar); [void]$g.Children.Add($ic); [void]$g.Children.Add($lb)
  $b.Content = $g
  $b.Add_Click({ Show-Page $this.Tag })
  [void]$navPanel.Children.Add($b)
  $script:Nav[$pageName] = @{ Btn = $b; Bar = $bar }
}
function Show-Page([string]$name) {
  foreach ($n in $Pages.Keys) {
    $sel = ($n -eq $name)
    $script:Window.FindName($Pages[$n][1]).Visibility = $(if ($sel) { 'Visible' } else { 'Collapsed' })
    $nb = $script:Nav[$n]
    $nb.Bar.Visibility = $(if ($sel) { 'Visible' } else { 'Hidden' })
    if ($sel) {
      $nb.Btn.SetResourceReference([System.Windows.Controls.Control]::BackgroundProperty, 'Chip')
      $nb.Btn.SetResourceReference([System.Windows.Controls.Control]::ForegroundProperty, 'Text')
    } else {
      $nb.Btn.Background = [System.Windows.Media.Brushes]::Transparent
      $nb.Btn.SetResourceReference([System.Windows.Controls.Control]::ForegroundProperty, 'Sub')
    }
  }
  $script:Window.FindName('PageTitle').Text = $name
  $script:CurrentPage = $name
  if ($name -eq 'Home') { $script:ChartKey = '' }
}
Show-Page 'Home'
$script:Window.Add_PreviewKeyDown({
  param($sender, $e)
  if ([System.Windows.Input.Keyboard]::Modifiers -eq [System.Windows.Input.ModifierKeys]::Control) {
    $i = [array]::IndexOf(@('D1', 'D2', 'D3', 'D4', 'D5', 'NumPad1', 'NumPad2', 'NumPad3', 'NumPad4', 'NumPad5'), "$($e.Key)")
    if ($i -ge 0) { Show-Page (@($Pages.Keys)[$i % 5]); $e.Handled = $true }
  }
})

# Fill in the system card
$specGrid = $script:Window.FindName('SpecGrid')
$rowN = 0
foreach ($k in $Sys.Keys) {
  [void]$specGrid.RowDefinitions.Add((New-Object System.Windows.Controls.RowDefinition))
  $l = New-Object System.Windows.Controls.TextBlock
  $l.Text = $k; $l.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Dim'); $l.FontSize = 13; $l.Margin = New-Object System.Windows.Thickness(0,3,0,3)
  $v = New-Object System.Windows.Controls.TextBlock
  $v.Text = $Sys[$k]; $v.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Text'); $v.FontSize = 13; $v.TextWrapping = 'Wrap'; $v.Margin = New-Object System.Windows.Thickness(0,3,0,3)
  [System.Windows.Controls.Grid]::SetRow($l, $rowN); [System.Windows.Controls.Grid]::SetRow($v, $rowN)
  [System.Windows.Controls.Grid]::SetColumn($v, 1)
  [void]$specGrid.Children.Add($l); [void]$specGrid.Children.Add($v)
  $rowN++
}
$checkPanel = $script:Window.FindName('CheckPanel')
function Build-Chips {
  $checkPanel.Children.Clear()
  foreach ($c in $script:Checks) {
    $chip = New-Object System.Windows.Controls.Border
    $chip.SetResourceReference([System.Windows.Controls.Border]::BackgroundProperty, 'Chip')
    $chip.SetResourceReference([System.Windows.Controls.Border]::CornerRadiusProperty, 'ChipRadius')
    $chip.Padding = New-Object System.Windows.Thickness(10,4,10,4); $chip.Margin = New-Object System.Windows.Thickness(0,0,6,6)
    $tip = $c.Tip
    if ($c.Fix) {
      $tip += "`n`n" + $c.FixLabel
      $chip.Cursor = [System.Windows.Input.Cursors]::Hand
      $chip.SetResourceReference([System.Windows.Controls.Border]::BorderBrushProperty, 'Sub'); $chip.BorderThickness = New-Object System.Windows.Thickness(1)
      $chip.Tag = $c
      $chip.Add_MouseLeftButtonUp({ Invoke-Fix $this.Tag })
    }
    $chip.ToolTip = $tip
    $sp = New-Object System.Windows.Controls.StackPanel; $sp.Orientation = 'Horizontal'
    $dot = New-Object System.Windows.Shapes.Ellipse; $dot.Width = 8; $dot.Height = 8; $dot.VerticalAlignment = 'Center'
    $dot.Margin = New-Object System.Windows.Thickness(0,0,6,0)
    switch ($c.State) {
      'ok'    { $dot.Fill = $script:Brush.Green }
      'warn'  { $dot.Fill = $script:Brush.Amber }
      default { $dot.SetResourceReference([System.Windows.Shapes.Shape]::FillProperty, 'Accent') }
    }
    $t = New-Object System.Windows.Controls.TextBlock; $t.Text = $c.Text; $t.FontSize = 11; $t.Opacity = 0.9
    $t.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Text')
    [void]$sp.Children.Add($dot); [void]$sp.Children.Add($t); $chip.Child = $sp
    [void]$checkPanel.Children.Add($chip)
  }
  $warns = @($script:Checks | Where-Object { $_.State -eq 'warn' }).Count
  $hd = $script:Window.FindName('HealthDot'); $ht = $script:Window.FindName('HealthText')
  if ($warns -eq 0) { $hd.Fill = $script:Brush.Green; $ht.Text = 'All checks passed' }
  else { $hd.Fill = $script:Brush.Amber; $ht.Text = "$warns check$(if ($warns -ne 1) { 's' }) need attention" }
}
function Invoke-Fix($c) {
  try { & $c.Fix; Write-Log "Fix applied: $($c.Text)" } catch { Write-Log "Fix failed: $($_.Exception.Message)" }
  Update-Checks
}
function Update-Checks {
  $script:Checks = Get-Checks
  $script:LastCheck = Get-Date
  Build-Chips
}
Build-Chips
$script:Window.FindName('HealthBtn').Add_Click({ Show-Page 'System' })
$script:Window.FindName('ShaderBtn').Add_Click({ Clear-ShaderCache })
$script:Window.FindName('CopyBtn').Add_Click({
  $txt = ($Sys.Keys | ForEach-Object { "{0}: {1}" -f $_, $Sys[$_] }) -join "`r`n"
  [System.Windows.Clipboard]::SetText($txt)
  Write-Log 'System specs copied to clipboard'
})

# ======================= Logic =======================
function Write-Log([string]$msg) {
  $script:LogBox.AppendText(("[{0:HH:mm:ss}] {1}`r`n" -f (Get-Date), $msg))
  $script:LogBox.ScrollToEnd()
}

function Get-CoreMode($p) {
  $m = switch ($p.Cores) { 'VCache' { 'Best' } 'Frequency' { 'Other' } default { $p.Cores } }
  if ($m -eq 'Other' -and [CpuTopo]::Kind -ne 'DualX3D') { $m = 'Best' }
  if (-not $script:CanPin) { $m = 'All' }
  return $m
}

function Get-Summary($p) {
  if ($p.AntiCheat) {
    $s = 'Anti-cheat safe mode'
    if ($p.LaunchPriority) { $s += "  |  starts at $(if ($p.LaunchPriority -eq 'AboveNormal') { 'Above Normal' } else { $p.LaunchPriority }) priority" }
    else { $s += ' (system tweaks only)' }
  }
  else {
    $cores = switch (Get-CoreMode $p) { 'Best' { $script:BestLabel } 'Other' { $script:OtherLabel } default { 'All cores' } }
    $prio = if ($p.Priority -eq 'AboveNormal') { 'Above Normal' } else { $p.Priority }
    $s = "$cores  |  $prio priority"
  }
  $pm = Get-Purge $p
  if ($pm -gt 0) { $s += "  |  RAM cleanup every $($pm)m" }
  if ($p.Boost -contains 'OVRServer_x64') { $s += '  |  VR runtime boost' }
  elseif ($p.Boost.Count -gt 0) { $s += '  |  helper boost' }
  return $s
}

function Update-PathText($name) {
  $r = $script:Rows[$name]
  $path = $script:Paths[$name]
  $txt = if ($path) { 'Launches: ' + [IO.Path]::GetFileName($path) } else { 'No launcher set - click SET' }
  $pt = [double]$script:Playtime[$name]
  if ($pt -ge 1) { $txt += '   |   ' + (Format-Playtime $pt) }
  $r.PathText.Text = $txt
}

# Background apps (plus your GPU maker's helper apps)
function Get-BackgroundList {
  $l = @($BackgroundApps)
  if ($script:Settings.VendorApps) { foreach ($v in $gpuVendors) { if ($VendorApps.ContainsKey($v)) { $l += $VendorApps[$v] } } }
  return @($l | Where-Object { $_ } | Select-Object -Unique)
}
# Cores background apps are moved to while a game runs (Intel E-cores / AMD non-V-Cache CCD)
function Get-BackgroundMask {
  if ($script:Settings.BackgroundCores -eq 'off' -or -not $script:CanPin) { return [int64]0 }
  if (($topo -eq 'Hybrid' -or $topo -eq 'DualX3D') -and [CpuTopo]::OtherMask -ne 0) { return [int64][CpuTopo]::OtherMask }
  return [int64]0
}
$script:SavedAff = @{}
function Set-BackgroundCores($names) {
  $mask = Get-BackgroundMask
  if ($mask -eq 0) { return 0 }
  $n = 0
  foreach ($nm in $names) {
    foreach ($pr in @(Get-Process -Name $nm -ErrorAction SilentlyContinue)) {
      if ($pr.Id -eq $PID -or $script:SavedAff.ContainsKey($pr.Id)) { continue }
      try { $orig = $pr.ProcessorAffinity; $pr.ProcessorAffinity = [IntPtr]$mask; $script:SavedAff[$pr.Id] = $orig; $n++ } catch {}
    }
  }
  return $n
}

# Clears DirectX + your GPU maker's shader caches (rebuilt on next game launch)
function Clear-ShaderCache {
  if ($script:Active) { Write-Log "Close $($script:Active.Name) first - its shader cache is in use"; return }
  $q = [System.Windows.MessageBox]::Show("Clear the DirectX and $(($gpuVendors | Where-Object { $_ -ne 'Other' }) -join ' / ') shader caches?`n`nThis fixes stutter or crashes after a driver update. The first launch of each game afterwards may stutter for a minute while shaders rebuild.", 'Project OptM', 'YesNo', 'Question')
  if ($q -ne 'Yes') { return }
  $dirs = @("$env:LOCALAPPDATA\D3DSCache")
  if ($gpuVendors -contains 'AMD')    { $dirs += @("$env:LOCALAPPDATA\AMD\DxCache", "$env:LOCALAPPDATA\AMD\DxcCache", "$env:LOCALAPPDATA\AMD\VkCache", "$env:LOCALAPPDATA\AMD\GLCache") }
  if ($gpuVendors -contains 'NVIDIA') { $dirs += @("$env:LOCALAPPDATA\NVIDIA\DXCache", "$env:LOCALAPPDATA\NVIDIA\GLCache", "$env:ProgramData\NVIDIA Corporation\NV_Cache") }
  if ($gpuVendors -contains 'Intel')  { $dirs += @("$env:LOCALAPPDATA\Intel\ShaderCache", "$env:USERPROFILE\AppData\LocalLow\Intel\ShaderCache") }
  $script:Window.Cursor = [System.Windows.Input.Cursors]::Wait
  $freed = [int64]0; $files = 0; $skipped = 0
  foreach ($d in $dirs) {
    if (-not (Test-Path -LiteralPath $d)) { continue }
    foreach ($f in @(Get-ChildItem -LiteralPath $d -Recurse -File -Force -ErrorAction SilentlyContinue)) {
      try { $len = $f.Length; Remove-Item -LiteralPath $f.FullName -Force -ErrorAction Stop; $freed += $len; $files++ } catch { $skipped++ }
    }
  }
  $script:Window.Cursor = $null
  $msg = "Shader cache cleared: $files files, $([math]::Round($freed / 1MB)) MB freed"
  if ($skipped -gt 0) { $msg += " ($skipped in use, skipped)" }
  Write-Log $msg
}

# What OptM does for this particular hardware (shown on the System tab)
function Get-VendorLines {
  $l = @()
  $bgMask = Get-BackgroundMask
  switch ($topo) {
    'DualX3D' {
      $t = "Games run on the V-Cache cores (CPU $(Format-Mask ([CpuTopo]::BestMask)))"
      if ($bgMask -ne 0) { $t += ", and background apps move to the other CCD (CPU $(Format-Mask $bgMask)) while you play" }
      $l += ,@($cpuVendor, "$t. Balanced power plan is kept so AMD's core parking works.")
    }
    'Hybrid' {
      $t = "Games run on the P-cores (CPU $(Format-Mask ([CpuTopo]::BestMask)))"
      if ($bgMask -ne 0) { $t += ", and background apps move to the E-cores (CPU $(Format-Mask $bgMask)) while you play" }
      $l += ,@($cpuVendor, "$t.")
    }
    'SingleX3D' { $l += ,@($cpuVendor, 'Every core has the extra 3D V-Cache, so no core pinning is needed.') }
    'DualCCD'   { $l += ,@($cpuVendor, 'Two CCDs without V-Cache - Windows schedules them well on its own, so no pinning is needed.') }
    default     { $l += ,@($cpuVendor, 'Single core cluster - no core pinning needed.') }
  }
  if ($cpuVendor -eq 'Intel' -and $cpuName -match '\bi[579]-1[34]\d{3}') { $l += ,@('Intel', 'Microcode is checked for the 13th/14th gen stability fix.') }
  foreach ($v in $gpuVendors) {
    $apps = if ($VendorApps.ContainsKey($v) -and $script:Settings.VendorApps) { $VendorApps[$v] -join ', ' } else { $null }
    switch ($v) {
      'AMD'    { $t = 'Radeon: driver age is checked.';     if ($apps) { $t = "Radeon: Adrenalin helpers ($apps) get low priority while you play; driver age is checked." } }
      'NVIDIA' { $t = 'GeForce: driver age is checked.';    if ($apps) { $t = "GeForce: NVIDIA app and overlay get low priority while you play; driver age and GPU scheduling (for DLSS Frame Generation) are checked." } }
      'Intel'  { $t = 'Intel graphics: driver age is checked.'; if ($apps) { $t = "Intel graphics: Intel Graphics Software gets low priority while you play; driver age is checked." } }
      default  { $t = $null }
    }
    if ($t) { $l += ,@($v, $t) }
  }
  if ($gpuList.Count -ge 2 -and $script:Settings.ForceGpu) { $l += ,@((Get-GpuVendor $gpuList[0]), "Games are locked to $($gpuList[0].Name), not the integrated graphics.") }
  return ,$l
}
function Update-VendorPanel {
  $vp = $script:Window.FindName('VendorPanel')
  if (-not $vp) { return }
  $vp.Children.Clear()
  $vendorLines = Get-VendorLines
  foreach ($pair in $vendorLines) {
    $g = New-Object System.Windows.Controls.Grid
    $g.Margin = New-Object System.Windows.Thickness(0,0,0,10)
    $c1 = New-Object System.Windows.Controls.ColumnDefinition; $c1.Width = New-Object System.Windows.GridLength(72)
    [void]$g.ColumnDefinitions.Add($c1); [void]$g.ColumnDefinitions.Add((New-Object System.Windows.Controls.ColumnDefinition))
    $badge = New-Object System.Windows.Controls.Border
    $badge.CornerRadius = New-Object System.Windows.CornerRadius(4); $badge.Padding = New-Object System.Windows.Thickness(6,2,6,2)
    $badge.HorizontalAlignment = 'Left'; $badge.VerticalAlignment = 'Top'
    $vc = if ($VendorColors.ContainsKey($pair[0])) { $VendorColors[$pair[0]] } else { $VendorColors['Other'] }
    $badge.Background = [System.Windows.Media.Brush](New-Object System.Windows.Media.SolidColorBrush ([System.Windows.Media.ColorConverter]::ConvertFromString($vc)))
    $bt = New-Object System.Windows.Controls.TextBlock; $bt.Text = $pair[0]; $bt.FontSize = 11; $bt.FontWeight = 'Bold'; $bt.Foreground = [System.Windows.Media.Brushes]::White
    $badge.Child = $bt
    $tx = New-Object System.Windows.Controls.TextBlock; $tx.Text = $pair[1]; $tx.FontSize = 13; $tx.TextWrapping = 'Wrap'
    $tx.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Text')
    [System.Windows.Controls.Grid]::SetColumn($tx, 1)
    [void]$g.Children.Add($badge); [void]$g.Children.Add($tx)
    [void]$vp.Children.Add($g)
  }
}

function Clear-Standby {
  try {
    $res = [GameOptMem]::PurgeStandby()
    if ($res -eq 0) { Write-Log '  Cleared standby RAM'; $script:SessionCleanups++ }
    else { Write-Log ("  Standby RAM clear failed (0x{0:X8})" -f $res) }
  } catch { Write-Log '  Standby RAM clear unavailable' }
}

function Set-Priority($names, [string]$prio) {
  foreach ($n in $names) {
    Get-Process -Name $n -ErrorAction SilentlyContinue | ForEach-Object {
      if (-not $script:Saved.ContainsKey($_.Id)) {
        try { $orig = $_.PriorityClass; $_.PriorityClass = $prio; $script:Saved[$_.Id] = $orig } catch {}
      }
    }
  }
}

# Session history (history.csv) - playtime for every session, FPS when the graph was on
function Save-SessionHistory([string]$game, $ts, $fs) {
  $mins = [math]::Round($ts.TotalMinutes, 1)
  if ($mins -lt 1) { return '' }
  $hasFps = $fs -and $fs[0] -gt 0
  $cmp = ''
  try {
    if ($hasFps -and $mins -ge 2 -and (Test-Path -LiteralPath $HistoryPath)) {
      $prev = @(Import-Csv -LiteralPath $HistoryPath | Where-Object {
        $a = 0.0; $m = 0.0
        $_.Game -eq $game -and [double]::TryParse("$($_.AvgFps)", [ref]$a) -and $a -gt 0 -and [double]::TryParse("$($_.Minutes)", [ref]$m) -and $m -ge 2
      }) | Select-Object -Last 1
      if ($prev) {
        $dAvg = [math]::Round($fs[0] - [double]$prev.AvgFps)
        $dLow = [math]::Round($fs[1] - [double]$prev.Low1)
        $cmp = " ({0:+0;-0;+0} avg, {1:+0;-0;+0} low vs last session)" -f $dAvg, $dLow
      }
    }
    [pscustomobject]@{
      Date = (Get-Date).ToString('yyyy-MM-dd HH:mm'); Game = $game; Minutes = $mins
      AvgFps = $(if ($hasFps) { [math]::Round($fs[0], 1) } else { '' })
      Low1   = $(if ($hasFps) { [math]::Round($fs[1], 1) } else { '' })
      Version = $AppVersion
    } | Export-Csv -LiteralPath $HistoryPath -Append -NoTypeInformation
  } catch {}
  Update-Playtime
  return $cmp
}
function Update-Playtime {
  $script:Playtime = @{}
  if (-not (Test-Path -LiteralPath $HistoryPath)) { return }
  try {
    foreach ($r in (Import-Csv -LiteralPath $HistoryPath)) {
      $m = 0.0
      if ([double]::TryParse("$($r.Minutes)", [ref]$m)) { $script:Playtime[$r.Game] = [double]$script:Playtime[$r.Game] + $m }
    }
  } catch {}
  if (Get-Command Update-HomeLists -ErrorAction SilentlyContinue) { try { Update-HomeLists } catch {} }
}
# Home page: recent sessions + most played
function New-Text([string]$text, [double]$size, [string]$res) {
  $t = New-Object System.Windows.Controls.TextBlock
  $t.Text = $text; $t.FontSize = $size; $t.TextTrimming = 'CharacterEllipsis'
  $t.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, $res)
  return $t
}
function Update-HomeLists {
  $rp = $script:Window.FindName('RecentPanel'); $tp = $script:Window.FindName('TopPanel')
  if (-not $rp) { return }
  $rp.Children.Clear(); $tp.Children.Clear()
  $rows = @()
  if (Test-Path -LiteralPath $HistoryPath) { try { $rows = @(Import-Csv -LiteralPath $HistoryPath) } catch {} }
  $inv = [Globalization.CultureInfo]::InvariantCulture
  if ($rows.Count -eq 0) { [void]$rp.Children.Add((New-Text 'No sessions yet - play a game and it shows up here.' 12 'Dim')) }
  $recent = @($rows | Select-Object -Last 12)
  [array]::Reverse($recent)
  foreach ($r in $recent) {
    $g = New-Object System.Windows.Controls.Grid
    $g.Margin = New-Object System.Windows.Thickness(0,0,0,10)
    [void]$g.ColumnDefinitions.Add((New-Object System.Windows.Controls.ColumnDefinition))
    $c2 = New-Object System.Windows.Controls.ColumnDefinition; $c2.Width = [System.Windows.GridLength]::Auto; [void]$g.ColumnDefinitions.Add($c2)
    $left = New-Object System.Windows.Controls.StackPanel
    $n = New-Text "$($r.Game)" 13 'Text'; $n.FontWeight = 'SemiBold'
    $d = [datetime]::MinValue
    $when = if ([datetime]::TryParseExact("$($r.Date)", 'yyyy-MM-dd HH:mm', $inv, 'None', [ref]$d)) { $d.ToString('MMM d, h:mm tt', $inv) } else { "$($r.Date)" }
    $a = 0.0
    if ([double]::TryParse("$($r.AvgFps)", [ref]$a) -and $a -gt 0) { $when += "   |   avg $([math]::Round($a)) FPS, 1% low $([math]::Round([double]$r.Low1))" }
    [void]$left.Children.Add($n); [void]$left.Children.Add((New-Text $when 11 'Dim'))
    $m = 0.0; [void][double]::TryParse("$($r.Minutes)", [ref]$m)
    $durText = if ($m -ge 60) { '{0}h {1}m' -f [math]::Floor($m / 60), [math]::Floor($m % 60) } else { '{0}m' -f [math]::Round($m) }
    $dur = New-Text $durText 12 'Sub'; $dur.VerticalAlignment = 'Center'; $dur.Margin = New-Object System.Windows.Thickness(12,0,0,0)
    [System.Windows.Controls.Grid]::SetColumn($dur, 1)
    [void]$g.Children.Add($left); [void]$g.Children.Add($dur)
    [void]$rp.Children.Add($g)
  }
  $top = @($script:Playtime.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 5)
  if ($top.Count -eq 0) { [void]$tp.Children.Add((New-Text 'Nothing yet' 12 'Dim')) }
  $max = if ($top.Count -gt 0) { [double]$top[0].Value } else { 1 }
  foreach ($e in $top) {
    $row = New-Object System.Windows.Controls.StackPanel; $row.Margin = New-Object System.Windows.Thickness(0,0,0,10)
    $hdr = New-Object System.Windows.Controls.Grid
    $nm = New-Text "$($e.Key)" 12 'Text'; $nm.Margin = New-Object System.Windows.Thickness(0,0,56,0)
    $hv = New-Text ('{0:0.#}h' -f ([double]$e.Value / 60)) 12 'Sub'; $hv.HorizontalAlignment = 'Right'
    [void]$hdr.Children.Add($nm); [void]$hdr.Children.Add($hv)
    $track = New-Object System.Windows.Controls.Grid; $track.Height = 4; $track.Margin = New-Object System.Windows.Thickness(0,5,0,0)
    $frac = [math]::Max(0.03, [double]$e.Value / $max)
    $ca = New-Object System.Windows.Controls.ColumnDefinition; $ca.Width = New-Object System.Windows.GridLength($frac, [System.Windows.GridUnitType]::Star)
    $cb = New-Object System.Windows.Controls.ColumnDefinition; $cb.Width = New-Object System.Windows.GridLength((1 - $frac + 0.0001), [System.Windows.GridUnitType]::Star)
    [void]$track.ColumnDefinitions.Add($ca); [void]$track.ColumnDefinitions.Add($cb)
    $bgBar = New-Object System.Windows.Controls.Border; $bgBar.CornerRadius = New-Object System.Windows.CornerRadius(2)
    $bgBar.SetResourceReference([System.Windows.Controls.Border]::BackgroundProperty, 'Chip')
    [System.Windows.Controls.Grid]::SetColumnSpan($bgBar, 2)
    $fill = New-Object System.Windows.Controls.Border; $fill.CornerRadius = New-Object System.Windows.CornerRadius(2)
    $fill.SetResourceReference([System.Windows.Controls.Border]::BackgroundProperty, 'Accent')
    [void]$track.Children.Add($bgBar); [void]$track.Children.Add($fill)
    [void]$row.Children.Add($hdr); [void]$row.Children.Add($track)
    [void]$tp.Children.Add($row)
  }
}

function Format-Playtime([double]$mins) {
  if ($mins -ge 60) { return ('{0:0.#}h played' -f ($mins / 60)) }
  return ('{0:0}m played' -f $mins)
}

function Format-Duration($ts) {
  if ($ts.TotalHours -ge 1) { return "{0}h {1}m" -f [int][math]::Floor($ts.TotalHours), $ts.Minutes }
  return "{0}m" -f [int][math]::Max(1, [math]::Round($ts.TotalMinutes))
}

# Windows Update pause (services are restarted afterwards)
function Get-PauseList {
  $l = @()
  if ($script:Settings.PauseUpdates) { $l += $UpdateServices }
  $l += @($script:Settings.PauseServices)
  return @($l | Where-Object { $_ } | Select-Object -Unique)
}
function Suspend-Updates {
  $before = $script:StoppedSvcs.Count
  foreach ($n in (Get-PauseList)) {
    $svc = Get-Service -Name $n -ErrorAction SilentlyContinue
    if ($svc -and $svc.Status -eq 'Running') {
      try {
        Stop-Service -Name $n -Force -NoWait -ErrorAction Stop
        if ($script:StoppedSvcs -notcontains $n) { $script:StoppedSvcs += $n }
      } catch {}
    }
  }
  if ($script:StoppedSvcs.Count -ne $before) { Save-Config }
}
function Resume-Updates {
  if ($script:StoppedSvcs.Count -eq 0) { return }
  foreach ($n in $script:StoppedSvcs) { try { Start-Service -Name $n -ErrorAction Stop } catch {} }
  $script:StoppedSvcs = @()
  Save-Config
}

# Power plan switch (non-X3D, or power_plan = high)
function Get-ActivePlan {
  $o = (powercfg /getactivescheme) -join ' '
  if ($o -match '([0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12})') { return $Matches[1] }
}
function Set-GamingPowerPlan {
  $mode = $script:Settings.PowerPlan
  if ($mode -eq 'off' -or ($mode -eq 'auto' -and $topo -eq 'DualX3D')) { return }
  $all = (powercfg /list) -join ' '
  $target = $null
  foreach ($g in @('8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c', 'e9a42b02-d5df-448d-aa00-03f14749eb61')) { if (-not $target -and $all -match $g) { $target = $g } }
  $cur = Get-ActivePlan
  if (-not $target -or -not $cur -or $cur -eq $target) { return }
  $script:RestorePlan = $cur; Save-Config
  powercfg /setactive $target | Out-Null
  Write-Log '  Power plan: High performance (until the game closes)'
}
function Restore-PowerPlan {
  if ($script:RestorePlan) {
    powercfg /setactive $script:RestorePlan | Out-Null
    $script:RestorePlan = $null; Save-Config
  }
}

# Lock a game to the dedicated GPU when an iGPU is also active (Windows per-app setting)
function Set-DedicatedGpu($proc) {
  if (-not $script:Settings.ForceGpu -or $gpuList.Count -lt 2) { return }
  $path = try { $proc.Path } catch { $null }
  if (-not $path) { return }
  $key = 'HKEY_CURRENT_USER\Software\Microsoft\DirectX\UserGpuPreferences'
  if ([Microsoft.Win32.Registry]::GetValue($key, $path, $null) -ne 'GpuPreference=2;') {
    [Microsoft.Win32.Registry]::SetValue($key, $path, 'GpuPreference=2;')
    Write-Log "  $($proc.ProcessName): locked to $($gpuList[0].Name) (from next launch)"
  }
}

# Close apps listed in the profile / settings (a game's "keep" list is never closed)
function Close-Apps($names, $keep) {
  $gameNames = @($Profiles | ForEach-Object { $_.Exe })
  $keepNames = Expand-AppNames $keep
  $closed = @()
  foreach ($n in (Expand-AppNames $names)) {
    if ($ProtectedNames -contains $n -or $gameNames -contains $n -or $keepNames -contains $n) { continue }
    Get-Process -Name $n -ErrorAction SilentlyContinue | ForEach-Object {
      if ($_.Id -eq $PID) { return }
      $exePath = try { $_.Path } catch { $null }
      try {
        if ($_.MainWindowHandle -ne [IntPtr]::Zero) { [void]$_.CloseMainWindow() } else { $_.Kill() }
        if ($closed -notcontains $_.ProcessName) { $closed += $_.ProcessName }
        if ($exePath -and $script:ReopenPaths -notcontains $exePath -and $AllLauncherNames -notcontains $_.ProcessName) { $script:ReopenPaths += $exePath }
      } catch {}
    }
  }
  if ($closed.Count -gt 0) { Write-Log "  Closed: $($closed -join ', ')" }
}
function Restore-ClosedApps {
  if ($script:Settings.ReopenClosed -and $script:ReopenPaths.Count -gt 0) {
    foreach ($pth in $script:ReopenPaths) {
      if (-not (Get-Process | Where-Object { try { $_.Path -eq $pth } catch { $false } } | Select-Object -First 1)) {
        try { Start-Process -FilePath "$env:WINDIR\explorer.exe" -ArgumentList "`"$pth`"" } catch {}
      }
    }
    Write-Log "  Reopened: $(($script:ReopenPaths | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_) }) -join ', ')"
  }
  $script:ReopenPaths = @()
}

# Launch priority: Windows' own per-exe setting (Image File Execution Options\PerfOptions)
function Sync-LaunchPriority {
  $ifeo = 'SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options'
  $codes = @{ Normal = 2; AboveNormal = 6; High = 3 }
  $want = @{}
  foreach ($p in $Profiles) { if ($p.LaunchPriority) { foreach ($e in $p.Exe) { $want["$e.exe"] = $p.LaunchPriority } } }
  foreach ($exe in @($script:IfeoManaged)) {
    if ($want.ContainsKey($exe)) { continue }
    try {
      $k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("$ifeo\$exe\PerfOptions", $true)
      if ($k) {
        $k.DeleteValue('CpuPriorityClass', $false)
        $empty = ($k.ValueCount -eq 0 -and $k.SubKeyCount -eq 0); $k.Close()
        if ($empty) { [Microsoft.Win32.Registry]::LocalMachine.DeleteSubKey("$ifeo\$exe\PerfOptions", $false) }
      }
      $k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("$ifeo\$exe")
      if ($k) {
        $empty = ($k.ValueCount -eq 0 -and $k.SubKeyCount -eq 0); $k.Close()
        if ($empty) { [Microsoft.Win32.Registry]::LocalMachine.DeleteSubKey("$ifeo\$exe", $false) }
      }
      Write-Log "Launch priority removed: $exe"
    } catch {}
  }
  foreach ($exe in $want.Keys) {
    try {
      $cur = [Microsoft.Win32.Registry]::GetValue("HKEY_LOCAL_MACHINE\$ifeo\$exe\PerfOptions", 'CpuPriorityClass', $null)
      if ($cur -ne $codes[$want[$exe]]) {
        [Microsoft.Win32.Registry]::SetValue("HKEY_LOCAL_MACHINE\$ifeo\$exe\PerfOptions", 'CpuPriorityClass', [int]$codes[$want[$exe]], 'DWord')
        Write-Log "Launch priority set: $exe starts at $($want[$exe])"
      }
    } catch { Write-Log "Couldn't set launch priority for $exe" }
  }
  $script:IfeoManaged = @($want.Keys)
  Save-Config
}

function Restore-All {
  if (Get-Command Stop-FrameCapture -ErrorAction SilentlyContinue) { Stop-FrameCapture }
  foreach ($ecoPid in $script:EcoPids) { try { [void][PowerThrottle]::Set($ecoPid, $false) } catch {} }
  $script:EcoPids = @()
  Resume-Updates
  Restore-PowerPlan
  if (Get-Command Restore-ClosedApps -ErrorAction SilentlyContinue) { Restore-ClosedApps }
  foreach ($id in @($script:Saved.Keys)) {
    $pr = Get-Process -Id $id -ErrorAction SilentlyContinue
    if ($pr) { try { $pr.PriorityClass = $script:Saved[$id] } catch {} }
  }
  $script:Saved.Clear()
  foreach ($id in @($script:SavedAff.Keys)) {
    $pr = Get-Process -Id $id -ErrorAction SilentlyContinue
    if ($pr) { try { $pr.ProcessorAffinity = $script:SavedAff[$id] } catch {} }
  }
  $script:SavedAff.Clear()
  $script:Tuned.Clear()
}

function Tune-Game($proc, $prof) {
  $script:Tuned[$proc.Id] = $true
  if ($prof.AntiCheat) { return }
  $parts = @()
  try { $proc.PriorityClass = $prof.Priority; $parts += "priority $($prof.Priority)" }
  catch { $parts += 'priority unchanged' }
  $mode = Get-CoreMode $prof
  if ($script:CanPin -and $mode -ne 'All') {
    $mask  = if ($mode -eq 'Other') { [CpuTopo]::OtherMask } else { [CpuTopo]::BestMask }
    $label = if ($mode -eq 'Other') { $script:OtherLabel } else { $script:BestLabel }
    try { $proc.ProcessorAffinity = [IntPtr]$mask; $parts += "pinned to $label (CPU $(Format-Mask $mask))" }
    catch { $parts += 'core pinning unchanged' }
  }
  if ($prof.EcoQosOff) {
    if ([PowerThrottle]::Set($proc.Id, $true)) { $parts += 'efficiency mode off'; $script:EcoPids += $proc.Id }
  }
  Write-Log ("  {0}: {1}" -f $proc.ProcessName, ($parts -join ', '))
  Set-DedicatedGpu $proc
}

function Test-Running($prof, $running) {
  foreach ($e in $prof.Exe) { if ($running.ContainsKey($e)) { return $true } }
  return $false
}

function Enable-Profile($prof) {
  $script:Active = $prof
  Write-Log ">> $($prof.Name) detected"
  $script:SessionStart = Get-Date
  $script:SessionCleanups = 0
  if ($prof.AntiCheat) {
    Write-Log '  Anti-cheat game: process left untouched, system tweaks only'
    if ($prof.LaunchPriority) { Write-Log "  Started at $($prof.LaunchPriority) priority by Windows (launch priority)" }
  }
  Close-Apps (@($script:Settings.Close) + @($prof.Close)) $prof.Keep
  Set-Priority (Get-BackgroundList) 'BelowNormal'
  Write-Log '  Background apps set to low priority'
  $moved = Set-BackgroundCores (Get-BackgroundList)
  if ($moved -gt 0) { Write-Log "  Background apps moved to $($script:OtherLabel) (CPU $(Format-Mask (Get-BackgroundMask)))" }
  if ($prof.Boost.Count -gt 0) { Set-Priority $prof.Boost 'High'; Write-Log "  Boosted: $($prof.Boost -join ', ')" }
  if ((Get-PauseList).Count -gt 0) {
    Suspend-Updates
    $what = @()
    if ($script:Settings.PauseUpdates) { $what += 'Windows Update' }
    $what += @($script:Settings.PauseServices)
    Write-Log "  Paused: $($what -join ', ')"
  }
  Set-GamingPowerPlan
  Get-Process -Name $prof.Exe -ErrorAction SilentlyContinue | ForEach-Object { Tune-Game $_ $prof }
  if ((Get-Purge $prof) -gt 0 -or ($script:Settings.CleanupOnLaunch -and $ramGB -lt 48)) { Clear-Standby }
  $script:LastPurge = Get-Date
  [FrameStats]::ResetSession()
  Start-FrameCapture $prof
}

function Update-Status {
  if (-not $script:AutoOn) {
    $script:StatusDot.Fill = $script:Brush.Gray
    $script:StatusText.Text = 'Paused'
    $script:StatusSub.Text  = 'Auto-optimize is off. Games run with normal Windows settings.'
    $tip = 'Project OptM - paused'
  }
  elseif ($script:Active) {
    $script:StatusDot.Fill = if ($script:Active.AntiCheat) { $script:Brush.Amber } else { $script:Brush.Green }
    $script:StatusText.Text = "Optimizing: $($script:Active.Name)"
    $script:StatusSub.Text  = Get-Summary $script:Active
    $tip = "Optimizing: $($script:Active.Name)"
  }
  else {
    $script:StatusDot.SetResourceReference([System.Windows.Shapes.Shape]::FillProperty, 'Accent')
    $script:StatusText.Text = 'Watching for games'
    $script:StatusSub.Text  = 'Launch a game from here or anywhere else - it gets optimized automatically.'
    $tip = 'Project OptM - watching'
  }
  if ($tip.Length -gt 63) { $tip = $tip.Substring(0, 63) }
  if ($script:Tray) { $script:Tray.Text = $tip }
  $bgP = [System.Windows.Controls.Control]::BackgroundProperty; $fgP = [System.Windows.Controls.Control]::ForegroundProperty
  if ($script:AutoOn) { $script:AutoBtn.Content = 'AUTO-OPTIMIZE: ON';  $script:AutoBtn.SetResourceReference($bgP, 'Accent'); $script:AutoBtn.SetResourceReference($fgP, 'AccentText') }
  else                { $script:AutoBtn.Content = 'AUTO-OPTIMIZE: OFF'; $script:AutoBtn.SetResourceReference($bgP, 'Btn');    $script:AutoBtn.SetResourceReference($fgP, 'Text') }
}

function Invoke-Tick {
  if ($script:AutoUpdate -and $UpdateRepo -and -not $script:Active -and ((Get-Date) - $script:LastUpdateCheck).TotalHours -ge 6) { Find-Update $false }
  if (-not (Test-Path -LiteralPath $ProfilesPath)) { Update-Profiles; return }
  if ((Get-Item -LiteralPath $ProfilesPath).LastWriteTimeUtc -ne $script:ProfilesStamp) { Update-Profiles; return }
  $running = @{}
  Get-Process | ForEach-Object { $running[$_.ProcessName] = $true }

  foreach ($p in $Profiles) {
    $r = $script:Rows[$p.Name]
    if (Test-Running $p $running) {
      $r.Dot.Fill = $script:Brush.Green; $r.Play.Content = 'RUNNING'; $r.Play.IsEnabled = $false
    } else {
      $r.Dot.Fill = $script:Brush.Idle;  $r.Play.Content = 'PLAY';    $r.Play.IsEnabled = $true
    }
  }

  if ($script:AutoOn) {
    if (-not $script:Active) {
      foreach ($p in $Profiles) { if (Test-Running $p $running) { Enable-Profile $p; break } }
    }
    elseif (-not (Test-Running $script:Active $running)) {
      Restore-All
      $dur = Format-Duration ((Get-Date) - $script:SessionStart)
      $extra = ''
      $fs = [FrameStats]::Session()
      if ($fs[0] -gt 0) { $extra += ", avg $([math]::Round($fs[0])) FPS, 1% low $([math]::Round($fs[1]))" }
      $extra += Save-SessionHistory $script:Active.Name ((Get-Date) - $script:SessionStart) $fs
      if ($script:Rows.ContainsKey($script:Active.Name)) { Update-PathText $script:Active.Name }
      if ($script:SessionCleanups -gt 0) { $extra += ", RAM cleared $($script:SessionCleanups)x" }
      Write-Log "<< $($script:Active.Name) closed after $dur$extra - everything restored"
      $script:Active = $null
    }
    else {
      $a = $script:Active
      Get-Process -Name $a.Exe -ErrorAction SilentlyContinue | ForEach-Object {
        if (-not $script:Tuned.ContainsKey($_.Id)) { Tune-Game $_ $a }
      }
      Set-Priority (Get-BackgroundList) 'BelowNormal'
      [void](Set-BackgroundCores (Get-BackgroundList))
      if ($a.Boost.Count -gt 0) { Set-Priority $a.Boost 'High' }
      if ((Get-PauseList).Count -gt 0) { Suspend-Updates }
      $pm = Get-Purge $a
      if ($pm -gt 0 -and ((Get-Date) - $script:LastPurge).TotalMinutes -ge $pm) {
        Clear-Standby; $script:LastPurge = Get-Date
      }
    }
  }
  Update-Status
}

function Select-GamePath($name) {
  $dlg = New-Object Microsoft.Win32.OpenFileDialog
  $dlg.Title = "Pick the shortcut or .exe for $name"
  $dlg.Filter = 'Games and shortcuts (*.exe;*.lnk;*.url)|*.exe;*.lnk;*.url|All files (*.*)|*.*'
  $dlg.DereferenceLinks = $false
  $dlg.InitialDirectory = [Environment]::GetFolderPath('Desktop')
  if ($dlg.ShowDialog($script:Window)) {
    $script:Paths[$name] = $dlg.FileName
    Save-Config
    Update-PathText $name
    Write-Log "$name will launch: $([IO.Path]::GetFileName($dlg.FileName))"
    return $true
  }
  return $false
}

function Start-Game($name) {
  $path = $script:Paths[$name]
  if (-not $path -or -not (Test-Path -LiteralPath $path)) {
    if (-not (Select-GamePath $name)) { return }
    $path = $script:Paths[$name]
  }
  Write-Log "Launching $name..."
  # Launch through Explorer so the game runs as your normal user, not as admin
  Start-Process -FilePath "$env:WINDIR\explorer.exe" -ArgumentList "`"$path`""
}

# What a profile would do on this PC, without changing anything
function Get-Plan($p) {
  $l = @()
  if ($p.AntiCheat) {
    $l += 'Game process: not touched (anti-cheat safe mode)'
    if ($p.LaunchPriority) { $l += "Launch priority: $($p.LaunchPriority) (applied by Windows itself at start)" }
  } else {
    $l += "Game priority: $($p.Priority)"
    $mode = Get-CoreMode $p
    if ($mode -eq 'All') { $l += "CPU cores: all ($topoText - no pinning needed)" }
    else {
      $mask  = if ($mode -eq 'Other') { [CpuTopo]::OtherMask } else { [CpuTopo]::BestMask }
      $label = if ($mode -eq 'Other') { $script:OtherLabel } else { $script:BestLabel }
      $l += "CPU cores: pinned to $label (CPU $(Format-Mask $mask))"
    }
    if ($p.EcoQosOff) { $l += "Efficiency mode: blocked for the game" }
    if ($gpuList.Count -ge 2 -and $script:Settings.ForceGpu) { $l += "GPU: locked to $($gpuList[0].Name)" }
  }
  $bg = @(Get-BackgroundList)
  $bgm = Get-BackgroundMask
  if ($bgm -ne 0) { $l += "Background apps: moved to $($script:OtherLabel) (CPU $(Format-Mask $bgm))" }
  if ($bg.Count -gt 0) { $l += "Low priority: $($bg.Count) background apps ($(($bg | Select-Object -First 4) -join ', ')$(if ($bg.Count -gt 4) { ', ...' }))" }
  if ($p.Boost.Count -gt 0) { $l += "High priority: $($p.Boost -join ', ')" }
  $closeList = @(@($script:Settings.Close) + @($p.Close) | Where-Object { $_ })
  if ($closeList.Count -gt 0) {
    $l += "Closed at start: $($closeList -join ', ')$(if ($script:Settings.ReopenClosed) { ' (reopened after)' })"
  }
  if (@($p.Keep).Count -gt 0) { $l += "Never closed for this game: $(@($p.Keep) -join ', ')" }
  $paused = @()
  if ($script:Settings.PauseUpdates) { $paused += 'Windows Update' }
  $paused += @($script:Settings.PauseServices)
  if ($paused.Count -gt 0) { $l += "Paused while playing: $($paused -join ', ')" }
  $pp = $script:Settings.PowerPlan
  if ($pp -eq 'off') { $l += 'Power plan: unchanged' }
  elseif ($pp -eq 'auto' -and $topo -eq 'DualX3D') { $l += 'Power plan: unchanged (X3D needs Balanced)' }
  else { $l += 'Power plan: High performance while playing' }
  $pm = Get-Purge $p
  $atLaunch = $pm -gt 0 -or ($script:Settings.CleanupOnLaunch -and $ramGB -lt 48)
  if ($pm -gt 0) { $l += "RAM cleanup: at launch, then every $pm min" }
  elseif ($atLaunch) { $l += 'RAM cleanup: once at launch' }
  if ($script:FpsOn -and (Get-PresentMon)) { $l += 'FPS graph: on' }
  return $l
}
function Show-Plan([string]$name) {
  $p = $Profiles | Where-Object { $_.Name -eq $name } | Select-Object -First 1
  if (-not $p) { return }
  $lines = Get-Plan $p
  $txt = "When $name runs, Project OptM will:`n`n" + (($lines | ForEach-Object { "  -  $_" }) -join "`n") + "`n`nEverything is put back when the game closes."
  [void][System.Windows.MessageBox]::Show($txt, "Project OptM - $name")
}

# Panic: undo everything right now and stop optimizing
function Invoke-Panic {
  $was = $script:Active
  Restore-All
  $script:Active = $null
  $script:AutoOn = $false
  Save-Config
  Update-Status
  Write-Log '!! PANIC - everything restored and auto-optimize paused. Turn AUTO back on to resume.'
  if ($script:Tray) { $script:Tray.ShowBalloonTip(3000, 'Project OptM', 'Panic: everything restored. Auto-optimize is paused.', 'Warning') }
}

function Show-Main {
  $script:Window.Show()
  $script:Window.WindowState = 'Normal'
  [void]$script:Window.Activate()
}

# --- Build game tiles ---
function Build-Tiles {
$gameList.Children.Clear()
$script:Rows = @{}
foreach ($p in $Profiles) {
  $row = [System.Windows.Markup.XamlReader]::Parse($rowXaml)
  $row.FindName('Title').Text = $p.Name
  $row.FindName('Sub').Text   = Get-Summary $p
  $play = $row.FindName('PlayBtn'); $play.Tag = $p.Name
  $set  = $row.FindName('SetBtn');  $set.Tag  = $p.Name
  $play.Add_Click({ Start-Game $this.Tag })
  $set.Add_Click({ [void](Select-GamePath $this.Tag) })
  $info = $row.FindName('InfoBtn'); $info.Tag = $p.Name
  $info.Add_Click({ Show-Plan $this.Tag })
  $script:Rows[$p.Name] = @{ Dot = $row.FindName('Dot'); Play = $play; PathText = $row.FindName('PathText') }
  [void]$gameList.Children.Add($row)
  Update-PathText $p.Name
}
}

function Update-Profiles {
  try {
    Import-Profiles
  } catch {
    Write-Log "Couldn't read profiles.ini: $($_.Exception.Message) - keeping previous profiles"
    try { $script:ProfilesStamp = (Get-Item -LiteralPath $ProfilesPath).LastWriteTimeUtc } catch {}
    return
  }
  if ($script:Active) { Restore-All; $script:Active = $null }
  Build-Tiles
  Sync-LaunchPriority
  Update-Checks
  Update-VendorPanel
  if ($script:Timer) { $script:Timer.Interval = [TimeSpan]::FromSeconds($PollSeconds) }
  Write-Log "Profiles reloaded - $($Profiles.Count) games"
  foreach ($w in $script:ProfileWarnings) { Write-Log "  ! $w" }
}

# One-time import of play history from another optimizer (history-import.csv)
$importPath = Join-Path (Split-Path $HistoryPath) 'history-import.csv'
$script:ImportedRows = 0
if (Test-Path -LiteralPath $importPath) {
  try {
    $rows = @(Import-Csv -LiteralPath $importPath)
    $rows | Select-Object Date, Game, Minutes, AvgFps, Low1, Version | Export-Csv -LiteralPath $HistoryPath -Append -NoTypeInformation
    Move-Item -LiteralPath $importPath -Destination (Join-Path (Split-Path $HistoryPath) 'history-import.done.csv') -Force
    $script:ImportedRows = $rows.Count
  } catch {}
}
# Play history from the "Optimizer" app (AppData\Local\Optimizer\session_history.txt).
# First run imports every session; after that only sessions newer than the last import.
function Import-OptimizerHistory {
  $src = Join-Path $env:LOCALAPPDATA 'Optimizer\session_history.txt'
  if (-not (Test-Path -LiteralPath $src)) { return 0 }
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $mark = [datetime]::MinValue
  if ($script:OptImportMark) {
    try { $mark = [datetime]::ParseExact($script:OptImportMark, 'yyyy-MM-dd HH:mm:ss', $inv) } catch {}
  } elseif (Test-Path -LiteralPath $HistoryPath) {
    # sessions already brought in with a history-import.csv count as imported
    foreach ($r in @(Import-Csv -LiteralPath $HistoryPath | Where-Object { $_.Version -eq 'imported' })) {
      $d = [datetime]::MinValue
      if ([datetime]::TryParseExact("$($r.Date)", 'yyyy-MM-dd HH:mm', $inv, 'None', [ref]$d) -and $d.AddSeconds(59) -gt $mark) { $mark = $d.AddSeconds(59) }
    }
  }
  $exeMap = @{}
  foreach ($p in $Profiles) { foreach ($e in $p.Exe) { $exeMap[$e.ToLower()] = $p.Name } }
  $rows = @(); $newest = $mark
  foreach ($line in [IO.File]::ReadAllLines($src)) {
    if ($line -notmatch '^\[(.+?)\]\s*\|\s*(.+?)\s*\|\s*(.+)$') { continue }
    $stamp = $Matches[1]; $exe = $Matches[2].Trim(); $dur = $Matches[3]
    $when = [datetime]::MinValue
    if (-not [datetime]::TryParseExact(($stamp -replace '\s+', ' ').Trim(), 'ddd MMM d HH:mm:ss yyyy', $inv, 'None', [ref]$when)) { continue }
    if ($when -le $mark) { continue }
    $mins = 0.0
    if ($dur -match '(\d+)\s*h') { $mins += 60 * [int]$Matches[1] }
    if ($dur -match '(\d+)\s*m') { $mins += [int]$Matches[1] }
    if ($dur -match '(\d+)\s*s') { $mins += [int]$Matches[1] / 60 }
    if ($mins -lt 1) { continue }
    $key = ($exe -replace '\.exe$', '').ToLower()
    $game = if ($exeMap.ContainsKey($key)) { $exeMap[$key] } else { $exe -replace '\.exe$', '' }
    $rows += [pscustomobject]@{ Date = $when.ToString('yyyy-MM-dd HH:mm'); Game = $game; Minutes = [math]::Round($mins, 1); AvgFps = ''; Low1 = ''; Version = 'Optimizer' }
    if ($when -gt $newest) { $newest = $when }
  }
  if ($rows.Count -gt 0) {
    New-Item -ItemType Directory -Force -Path (Split-Path $HistoryPath) | Out-Null
    $rows | Export-Csv -LiteralPath $HistoryPath -Append -NoTypeInformation
  }
  if ($newest -gt $mark -or -not $script:OptImportMark) {
    $script:OptImportMark = $(if ($newest -gt [datetime]::MinValue) { $newest } else { Get-Date }).ToString('yyyy-MM-dd HH:mm:ss')
    Save-Config
  }
  return $rows.Count
}
$script:OptimizerImported = 0
try { $script:OptimizerImported = Import-OptimizerHistory } catch {}

Update-Playtime
Build-Tiles

# --- Buttons ---
$script:AutoBtn.Add_Click({
  $script:AutoOn = -not $script:AutoOn
  if (-not $script:AutoOn -and $script:Active) {
    Restore-All
    Write-Log "Auto-optimize off - $($script:Active.Name) restored to normal"
    $script:Active = $null
  } else { Write-Log ("Auto-optimize " + $(if ($script:AutoOn) { 'on' } else { 'off' })) }
  Save-Config
  Invoke-Tick
})
$openProfiles = {
  Initialize-ProfilesFile
  Start-Process -FilePath notepad.exe -ArgumentList "`"$ProfilesPath`""
}
$script:Window.FindName('EditBtn').Add_Click($openProfiles)
$script:Window.FindName('EditBtn2').Add_Click($openProfiles)
$script:Window.FindName('OpenDataBtn').Add_Click({ Open-Link (Split-Path $ProfilesPath) })

# ======================= Theme =======================
$Accents = [ordered]@{
  Blue = '#4F8BFF'; Crimson = '#E5484D'; Emerald = '#30C48D'; Violet = '#8E6CFF'
  Amber = '#F5A524'; Cyan = '#22C3E6'; Pink = '#F062A8'; Mono = '#D0D4DC'
}
$Backgrounds = [ordered]@{
  'Dark'       = @{ Bg = '#0F1115'; Card = '#181B22'; Card2 = '#12151B'; Btn = '#262B36'; Chip = '#222631'; Line = '#2A2F3A' }
  'Darker'     = @{ Bg = '#0A0B0E'; Card = '#121419'; Card2 = '#0D0F13'; Btn = '#1F222A'; Chip = '#1A1D24'; Line = '#23262E' }
  'OLED Black' = @{ Bg = '#000000'; Card = '#0B0B0D'; Card2 = '#050506'; Btn = '#1A1A1E'; Chip = '#141417'; Line = '#1E1E22' }
  'Slate'      = @{ Bg = '#161A22'; Card = '#1F2430'; Card2 = '#1A1E27'; Btn = '#2E3544'; Chip = '#2A303D'; Line = '#343B4A' }
}
$Corners = [ordered]@{ 'Rounded' = @(10, 6, 11); 'Soft' = @(5, 3, 5); 'Sharp' = @(0, 0, 0) }

function New-Brush([string]$hex) {
  $b = New-Object System.Windows.Media.SolidColorBrush ([System.Windows.Media.ColorConverter]::ConvertFromString($hex))
  $b.Freeze(); return $b
}
function Set-Resource([string]$key, $value) {
  if ($value -is [System.Management.Automation.PSObject]) { $value = $value.psobject.BaseObject }
  $script:Window.Resources.Remove($key)
  $script:Window.Resources.Add($key, $value)
}
function Get-TextOn([string]$hex) {
  $c = [System.Windows.Media.ColorConverter]::ConvertFromString($hex)
  $lum = (0.299 * $c.R + 0.587 * $c.G + 0.114 * $c.B) / 255
  if ($lum -gt 0.62) { return '#15171C' } else { return '#FFFFFF' }
}

# Logo drawn with GDI+ for the window + tray icons (same design as the top bar logo)
function New-LogoBitmap([int]$size, [string]$hex) {
  $bmp = New-Object System.Drawing.Bitmap $size, $size
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $k = [single]($size / 40.0)
  $rad = [single]([math]::Max(1, $Corners[$script:Theme.Corners][0]) * $k * 2)
  $w = [single]($size - 1)
  $path = New-Object System.Drawing.Drawing2D.GraphicsPath
  $path.AddArc([single]0, [single]0, $rad, $rad, [single]180, [single]90)
  $path.AddArc([single]($w - $rad), [single]0, $rad, $rad, [single]270, [single]90)
  $path.AddArc([single]($w - $rad), [single]($w - $rad), $rad, $rad, [single]0, [single]90)
  $path.AddArc([single]0, [single]($w - $rad), $rad, $rad, [single]90, [single]90)
  $path.CloseFigure()
  $fill = New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml('#0A0C10'))
  $g.FillPath($fill, $path)
  $accentCol = [System.Drawing.ColorTranslator]::FromHtml($hex)
  $edge = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(140, $accentCol), [single][math]::Max(1, $k * 0.8))
  $g.DrawPath($edge, $path); $edge.Dispose()
  $pen = New-Object System.Drawing.Pen ($accentCol, [single](4.6 * $k))
  $g.DrawEllipse($pen, [single](8.5 * $k), [single](8.5 * $k), [single](23 * $k), [single](23 * $k))
  $white = [System.Drawing.Color]::White
  $pen.Color = $white
  $pen.Width = [single](2.6 * $k)
  $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round; $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
  $g.DrawLine($pen, [single](20 * $k), [single](20 * $k), [single](24.6 * $k), [single](15.4 * $k))
  $dotBrush = New-Object System.Drawing.SolidBrush $white
  $g.FillEllipse($dotBrush, [single](17.9 * $k), [single](17.9 * $k), [single](4.2 * $k), [single](4.2 * $k))
  $g.Dispose(); $pen.Dispose(); $fill.Dispose(); $dotBrush.Dispose(); $path.Dispose()
  return $bmp
}
function Update-Logo {
  try {
    $small = New-LogoBitmap 32 $script:Theme.Accent
    $script:TrayIconObj = [System.Drawing.Icon]::FromHandle($small.GetHicon())
    if ($script:Tray) { $script:Tray.Icon = $script:TrayIconObj }
    $big = New-LogoBitmap 64 $script:Theme.Accent
    $script:Window.Icon = [System.Windows.Interop.Imaging]::CreateBitmapSourceFromHIcon($big.GetHicon(), [System.Windows.Int32Rect]::Empty, [System.Windows.Media.Imaging.BitmapSizeOptions]::FromEmptyOptions())
  } catch {}
}

function Set-Theme {
  try { Set-ThemeCore }
  catch {
    Write-Log "Theme error: $($_.Exception.Message) - using the default look"
    $script:Theme = $DefaultTheme.Clone()
    try { Set-ThemeCore } catch {}
  }
}
function Set-ThemeCore {
  $t = $script:Theme
  if (-not $Backgrounds.Contains($t.Bg)) { $t.Bg = 'Dark' }
  if (-not $Corners.Contains($t.Corners)) { $t.Corners = 'Rounded' }
  if ($t.Accent -notmatch '^#[0-9A-Fa-f]{6}$') { $t.Accent = '#4F8BFF' }
  $res = $script:Window.Resources
  $bgSet = $Backgrounds[$t.Bg]
  foreach ($k in $bgSet.Keys) { Set-Resource $k (New-Brush $bgSet[$k]) }
  Set-Resource 'Accent'     (New-Brush $t.Accent)
  Set-Resource 'AccentSoft' (New-Brush ('#38' + $t.Accent.Substring(1)))
  Set-Resource 'AccentText' (New-Brush (Get-TextOn $t.Accent))
  $cr = $Corners[$t.Corners]
  Set-Resource 'CardRadius' ([System.Windows.CornerRadius]::new([double]$cr[0]))
  Set-Resource 'BtnRadius'  ([System.Windows.CornerRadius]::new([double]$cr[1]))
  Set-Resource 'ChipRadius' ([System.Windows.CornerRadius]::new([double]$cr[2]))
  Update-Logo
  Build-ThemePanels
  $script:ChartKey = ''   # force graph redraw in the new colors
}

function New-OptionButton([string]$label, [bool]$selected) {
  $b = New-Object System.Windows.Controls.Button
  $b.Content = $label; $b.Padding = New-Object System.Windows.Thickness(12,6,12,6); $b.Margin = New-Object System.Windows.Thickness(0,0,6,6)
  if ($selected) {
    $b.SetResourceReference([System.Windows.Controls.Control]::BackgroundProperty, 'Accent')
    $b.SetResourceReference([System.Windows.Controls.Control]::ForegroundProperty, 'AccentText')
  }
  return $b
}

function Build-ThemePanels {
  $ap = $script:Window.FindName('AccentPanel'); $ap.Children.Clear()
  foreach ($name in $Accents.Keys) {
    $hex = $Accents[$name]
    $b = New-Object System.Windows.Controls.Button
    $b.Width = 30; $b.Height = 30; $b.Padding = New-Object System.Windows.Thickness(0); $b.Margin = New-Object System.Windows.Thickness(0,0,8,8)
    $b.Background = [System.Windows.Media.Brush](New-Brush $hex); $b.ToolTip = $name; $b.Tag = $hex
    if ($script:Theme.Accent -eq $hex) {
      $mark = [System.Windows.Shapes.Ellipse]::new(); $mark.Width = 10; $mark.Height = 10
      $mark.Fill = [System.Windows.Media.Brush](New-Brush (Get-TextOn $hex))
      $b.Content = $mark
    }
    $b.Add_Click({ $script:Theme.Accent = $this.Tag; Save-Config; Set-Theme })
    [void]$ap.Children.Add($b)
  }
  $script:Window.FindName('HexBox').Text = $script:Theme.Accent

  $bp = $script:Window.FindName('BgPanel'); $bp.Children.Clear()
  foreach ($name in $Backgrounds.Keys) {
    $b = New-OptionButton $name ($script:Theme.Bg -eq $name); $b.Tag = $name
    $b.Add_Click({ $script:Theme.Bg = $this.Tag; Save-Config; Set-Theme })
    [void]$bp.Children.Add($b)
  }
  $cp = $script:Window.FindName('CornerPanel'); $cp.Children.Clear()
  foreach ($name in $Corners.Keys) {
    $b = New-OptionButton $name ($script:Theme.Corners -eq $name); $b.Tag = $name
    $b.Add_Click({ $script:Theme.Corners = $this.Tag; Save-Config; Set-Theme })
    [void]$cp.Children.Add($b)
  }
}

$script:Window.FindName('HexBtn').Add_Click({
  $v = $script:Window.FindName('HexBox').Text.Trim()
  if (-not $v.StartsWith('#')) { $v = '#' + $v }
  if ($v -match '^#[0-9A-Fa-f]{6}$') { $script:Theme.Accent = $v.ToUpper(); Save-Config; Set-Theme }
  else { Write-Log "Color '$v' isn't valid - use #RRGGBB, like #FF6A00" }
})
$script:Window.FindName('ResetThemeBtn').Add_Click({ $script:Theme = $DefaultTheme.Clone(); Save-Config; Set-Theme })

# ======================= FPS graph =======================
$PmDir  = Join-Path $env:APPDATA 'ProjectOptM'
$script:FrameBuf  = New-Object 'System.Collections.Generic.List[double]'
$script:ChartKey  = ''
$script:CaptureOn = $false
$chart    = $script:Window.FindName('Chart')
$chartHost = $script:Window.FindName('ChartHost')
$chartMsg = $script:Window.FindName('ChartMsg')
$fpsText  = $script:Window.FindName('FpsText')
$lowText  = $script:Window.FindName('LowText')
$ftText   = $script:Window.FindName('FtText')
$fpsBtn   = $script:Window.FindName('FpsBtn')

function Get-PresentMon {
  $f = Get-ChildItem -LiteralPath $PmDir -Filter 'PresentMon*.exe' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if ($f) { return $f.FullName } else { return $null }
}

function Install-PresentMon {
  $q = [System.Windows.MessageBox]::Show("The FPS graph uses Intel PresentMon - a free, open-source frame capture tool (the same one many benchmarking apps use). It reads frame timing from Windows and never touches the game.`n`nDownload it from Intel's official GitHub now?", 'Project OptM', 'YesNo', 'Question')
  if ($q -ne 'Yes') { return }
  $script:Window.Cursor = [System.Windows.Input.Cursors]::Wait
  try {
    Write-Log 'Downloading Intel PresentMon from GitHub...'
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/GameTechDev/PresentMon/releases/latest' -Headers @{ 'User-Agent' = 'ProjectOptM' } -UseBasicParsing
    $asset = $rel.assets | Where-Object { $_.name -match '^PresentMon-[\d\.]+-x64\.exe$' } | Select-Object -First 1
    if (-not $asset) { $asset = $rel.assets | Where-Object { $_.name -match 'x64\.exe$' -and $_.name -notmatch 'Service|Setup|Install' } | Select-Object -First 1 }
    if (-not $asset) { throw "couldn't find the console app in the latest release" }
    New-Item -ItemType Directory -Force -Path $PmDir | Out-Null
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile (Join-Path $PmDir $asset.name) -UseBasicParsing
    Write-Log "PresentMon ready ($($asset.name)) - FPS graph enabled"
    $script:FpsOn = $true; Save-Config
    if ($script:Active) { Start-FrameCapture $script:Active }
  } catch {
    Write-Log "Download failed: $($_.Exception.Message)"
    Write-Log "  You can also download PresentMon-x.x.x-x64.exe yourself and put it in $PmDir"
  } finally { $script:Window.Cursor = $null }
  Update-FpsButton
}

function Update-FpsButton {
  $bgP = [System.Windows.Controls.Control]::BackgroundProperty; $fgP = [System.Windows.Controls.Control]::ForegroundProperty
  if (-not (Get-PresentMon)) { $fpsBtn.Content = 'Enable FPS graph'; $fpsBtn.SetResourceReference($bgP, 'Accent'); $fpsBtn.SetResourceReference($fgP, 'AccentText') }
  elseif ($script:FpsOn)     { $fpsBtn.Content = 'Graph: ON';        $fpsBtn.SetResourceReference($bgP, 'Btn');    $fpsBtn.SetResourceReference($fgP, 'Text') }
  else                       { $fpsBtn.Content = 'Graph: OFF';       $fpsBtn.SetResourceReference($bgP, 'Btn');    $fpsBtn.SetResourceReference($fgP, 'Sub') }
}
$fpsBtn.Add_Click({
  if (-not (Get-PresentMon)) { Install-PresentMon; return }
  $script:FpsOn = -not $script:FpsOn; Save-Config
  if ($script:FpsOn -and $script:Active) { Start-FrameCapture $script:Active }
  if (-not $script:FpsOn) { Stop-FrameCapture }
  Update-FpsButton
})

function Start-FrameCapture($prof) {
  $pm = Get-PresentMon
  if (-not $script:FpsOn -or -not $pm) { return }
  $script:FrameBuf.Clear()
  [FrameStats]::ResetSession()
  $names = [string[]]@($prof.Exe | ForEach-Object { "$_.exe" })
  if ([FrameMon]::Start($pm, $names)) { $script:CaptureOn = $true; Write-Log '  FPS capture started' }
  else { Write-Log "  FPS capture couldn't start: $([FrameMon]::LastError)" }
}
function Stop-FrameCapture {
  [FrameMon]::Stop()
  $script:CaptureOn = $false
}

function Set-ChartIdle([string]$msg) {
  if ($script:ChartKey -eq "idle:$msg") { return }
  $script:ChartKey = "idle:$msg"
  $chart.Children.Clear()
  $chartMsg.Text = $msg
  $fpsText.Text = '--'; $lowText.Text = '1% low      --'; $ftText.Text = 'Frametime   --'
}

function Add-ChartLine([double]$y, [double]$w, [string]$label) {
  $ln = New-Object System.Windows.Shapes.Line
  $ln.X1 = 0; $ln.X2 = $w; $ln.Y1 = $y; $ln.Y2 = $y; $ln.StrokeThickness = 1
  $ln.StrokeDashArray = New-Object System.Windows.Media.DoubleCollection (,[double[]]@(4, 4))
  $ln.SetResourceReference([System.Windows.Shapes.Shape]::StrokeProperty, 'Line')
  [void]$chart.Children.Add($ln)
  $tb = New-Object System.Windows.Controls.TextBlock
  $tb.Text = $label; $tb.FontSize = 10
  $tb.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Dim')
  [System.Windows.Controls.Canvas]::SetRight($tb, 6); [System.Windows.Controls.Canvas]::SetTop($tb, [math]::Max(0, $y - 14))
  [void]$chart.Children.Add($tb)
}

function Update-Chart {
  if (-not (Get-PresentMon)) { Set-ChartIdle 'Live FPS and frametime graph. Click "Enable FPS graph" to set it up.'; return }
  if (-not $script:FpsOn)    { Set-ChartIdle 'FPS graph is off'; return }
  if (-not $script:Active)   { Set-ChartIdle 'Start a game to see live FPS and frametimes'; return }

  $new = [FrameMon]::Drain()
  if ($new.Length -gt 0) {
    $script:FrameBuf.AddRange($new)
    [FrameStats]::AddSession($new)
    if ($script:FrameBuf.Count -gt 8000) { $script:FrameBuf.RemoveRange(0, $script:FrameBuf.Count - 8000) }
  }
  if ($script:FrameBuf.Count -lt 10) {
    if (-not [FrameMon]::Running -and [FrameMon]::LastError) { Set-ChartIdle "FPS capture stopped: $([FrameMon]::LastError)" }
    else { Set-ChartIdle 'Waiting for frames...' }
    return
  }
  if ($new.Length -eq 0 -and $script:ChartKey -eq 'live') { return }
  $script:ChartKey = 'live'
  $chartMsg.Text = ''

  $arr = $script:FrameBuf.ToArray()
  $st = [FrameStats]::Stats($arr)
  $fpsText.Text = [math]::Round($st[0])
  $lowText.Text = if ($st[1] -gt 0) { '1% low      {0}' -f [math]::Round($st[1]) } else { '1% low      --' }
  $ftText.Text  = 'Frametime   {0:N1} ms' -f $st[2]

  $w = $chartHost.ActualWidth; $h = $chartHost.ActualHeight
  if ($w -lt 20 -or $h -lt 20) { return }
  $colsN = [int][math]::Max(20, [math]::Floor($w / 2))
  $cols = [FrameStats]::Columns($arr, 8000, $colsN)

  $maxHz = if ($dispInfo[3] -gt 0) { $dispInfo[3] } else { 60 }
  $targetMs = 1000.0 / $maxHz
  $lowMs = if ($st[1] -gt 0) { 1000.0 / $st[1] } else { $st[2] }
  $need = [math]::Max($lowMs * 1.3, [math]::Max($targetMs * 1.6, $st[2] * 1.8))
  $ymax = 200
  foreach ($n in @(8.33, 16.67, 33.33, 50, 100, 200)) { if ($n -ge $need) { $ymax = $n; break } }

  $chart.Children.Clear()
  $pad = 6
  $plotH = $h - $pad * 2
  $toY = { param($ms) $pad + $plotH - ([math]::Min($ms, $ymax) / $ymax) * $plotH }
  if ($targetMs -lt $ymax) { Add-ChartLine (& $toY $targetMs) $w "$maxHz Hz" }
  if ($maxHz -ne 60 -and 16.67 -lt $ymax) { Add-ChartLine (& $toY 16.67) $w '60 FPS' }
  $top = New-Object System.Windows.Controls.TextBlock
  $top.Text = "{0:0.#} ms" -f $ymax; $top.FontSize = 10
  $top.SetResourceReference([System.Windows.Controls.TextBlock]::ForegroundProperty, 'Faint')
  [System.Windows.Controls.Canvas]::SetLeft($top, 6); [System.Windows.Controls.Canvas]::SetTop($top, 2)
  [void]$chart.Children.Add($top)

  $pts = New-Object System.Windows.Media.PointCollection
  $step = $w / ($colsN - 1)
  $first = -1
  for ($i = 0; $i -lt $colsN; $i++) {
    if ($cols[$i] -le 0) { continue }
    if ($first -lt 0) { $first = $i }
    $pts.Add((New-Object System.Windows.Point ($i * $step), (& $toY $cols[$i])))
  }
  if ($pts.Count -lt 2) { return }
  $area = $pts.Clone()
  $area.Add((New-Object System.Windows.Point ($w, ($h))))
  $area.Add((New-Object System.Windows.Point (($first * $step), ($h))))
  $poly = New-Object System.Windows.Shapes.Polygon
  $poly.Points = $area
  $poly.SetResourceReference([System.Windows.Shapes.Shape]::FillProperty, 'AccentSoft')
  [void]$chart.Children.Add($poly)
  $line = New-Object System.Windows.Shapes.Polyline
  $line.Points = $pts; $line.StrokeThickness = 1.6; $line.StrokeLineJoin = 'Round'
  $line.SetResourceReference([System.Windows.Shapes.Shape]::StrokeProperty, 'Accent')
  [void]$chart.Children.Add($line)
}

# ======================= Updates =======================
$script:UpdateInfo = $null
$script:UpdateStatus = ''
$script:LastUpdateCheck = (Get-Date).AddHours(-6).AddSeconds(8)   # first automatic check ~8s after start
$updateBtn = $script:Window.FindName('UpdateBtn')
$script:Window.FindName('SubTitle').Text = "v$AppVersion"

function Test-Newer([string]$tag) {
  try { return ([version]($tag.Trim().TrimStart('v', 'V')) -gt [version]$AppVersion) } catch { return $false }
}

function Update-AboutText {
  $script:Window.FindName('AboutVersion').Text = "Project OptM v$AppVersion"
  $st = if (-not $UpdateRepo) { 'Updates are off (no GitHub repo set at the top of the file).' }
        elseif ($script:UpdateInfo) { "Version $($script:UpdateInfo.Version) is available." }
        elseif ($script:UpdateStatus) { $script:UpdateStatus }
        else { "Updates from github.com/$UpdateRepo" }
  $script:Window.FindName('AboutStatus').Text = $st
  $ab = $script:Window.FindName('AutoUpdBtn')
  $ab.Content = if ($script:AutoUpdate) { 'Auto-check: ON' } else { 'Auto-check: OFF' }
  $script:Window.FindName('CheckUpdBtn').IsEnabled = [bool]$UpdateRepo
  $ab.IsEnabled = [bool]$UpdateRepo
}

function Find-Update([bool]$manual) {
  $script:LastUpdateCheck = Get-Date
  if (-not $UpdateRepo) { if ($manual) { Write-Log 'Updates are off - set $UpdateRepo at the top of ProjectOptM.ps1' }; return }
  if ($manual) { $script:Window.Cursor = [System.Windows.Input.Cursors]::Wait }
  try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $rel = Invoke-RestMethod -Uri "https://api.github.com/repos/$UpdateRepo/releases/latest" -UseBasicParsing -TimeoutSec 10 `
             -Headers @{ 'User-Agent' = 'ProjectOptM'; 'Accept' = 'application/vnd.github+json' }
    $wantExe = "$env:GO_SELF" -like '*.exe'
    $asset = $rel.assets | Where-Object { $_.name -eq $(if ($wantExe) { 'ProjectOptM.exe' } else { 'ProjectOptM.bat' }) } | Select-Object -First 1
    if (-not $asset -and -not $wantExe) { $asset = $rel.assets | Where-Object { $_.name -like '*.bat' } | Select-Object -First 1 }
    if ($asset -and (Test-Newer "$($rel.tag_name)")) {
      $ver = "$($rel.tag_name)".Trim().TrimStart('v', 'V')
      $isNew = -not $script:UpdateInfo -or $script:UpdateInfo.Version -ne $ver
      $script:UpdateInfo = @{ Version = $ver; Url = $asset.browser_download_url; Notes = "$($rel.body)"; Digest = "$($asset.digest)" }
      $updateBtn.Content = "Update to v$ver"
      $updateBtn.Visibility = 'Visible'
      if ($isNew) {
        Write-Log "Update available: v$ver - click 'Update to v$ver' at the top"
        if (-not $script:Window.IsVisible -and $script:Tray) { $script:Tray.ShowBalloonTip(4000, 'Project OptM', "Version $ver is available. Open the app to update.", 'Info') }
      }
    } else {
      $script:UpdateStatus = "Up to date (checked $((Get-Date).ToString('HH:mm')))"
      if ($manual) { Write-Log "You're on the latest version (v$AppVersion)" }
    }
  } catch {
    $script:UpdateStatus = "Couldn't check for updates"
    if ($manual) { Write-Log "Update check failed: $($_.Exception.Message)" }
  } finally { if ($manual) { $script:Window.Cursor = $null } }
  Update-AboutText
}

function Install-Update {
  $u = $script:UpdateInfo
  if (-not $u) { return }
  $notes = $u.Notes.Trim()
  if ($notes.Length -gt 700) { $notes = $notes.Substring(0, 700) + '...' }
  $msg = "Update Project OptM from v$AppVersion to v$($u.Version)?"
  if ($notes) { $msg += "`n`nWhat's new:`n$notes" }
  $msg += "`n`nThe app will restart. Your profiles, shortcuts and settings are kept."
  if ([System.Windows.MessageBox]::Show($msg, 'Project OptM', 'YesNo', 'Question') -ne 'Yes') { return }
  $script:Window.Cursor = [System.Windows.Input.Cursors]::Wait
  try {
    Write-Log "Downloading v$($u.Version)..."
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $isExe = "$env:GO_SELF" -like '*.exe'
    $ext = if ($isExe) { '.exe' } else { '.bat' }
    $tmp = Join-Path $env:TEMP "ProjectOptM-$($u.Version)$ext"
    Invoke-WebRequest -Uri $u.Url -OutFile $tmp -UseBasicParsing -TimeoutSec 120
    if ($u.Digest -match '^sha256:([0-9a-fA-F]{64})$') {
      $want = $Matches[1]
      if ((Get-FileHash -LiteralPath $tmp -Algorithm SHA256).Hash -ne $want) { throw 'the download was corrupted (checksum mismatch)' }
    }
    if ($isExe) {
      $bytes = [IO.File]::ReadAllBytes($tmp)
      if ($bytes.Length -lt 20000 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { throw "the downloaded file isn't a valid app" }
    } else {
      $text = [IO.File]::ReadAllText($tmp)
      if ($text -notmatch '<# : ---------- launcher' -or $text -notmatch '\$AppVersion\s*=') { throw "the downloaded file doesn't look like Project OptM" }
    }
    $bakDir = Join-Path $env:APPDATA 'ProjectOptM\backup'
    New-Item -ItemType Directory -Force -Path $bakDir | Out-Null
    Copy-Item -LiteralPath $env:GO_SELF -Destination (Join-Path $bakDir "ProjectOptM-v$AppVersion$ext") -Force
    if ($isExe) {
      # a running exe can't be overwritten, but it can be renamed out of the way
      $old = "$env:GO_SELF.old"
      Remove-Item -LiteralPath $old -Force -ErrorAction SilentlyContinue
      Move-Item -LiteralPath $env:GO_SELF -Destination $old -Force
      try { Copy-Item -LiteralPath $tmp -Destination $env:GO_SELF -Force }
      catch { Move-Item -LiteralPath $old -Destination $env:GO_SELF -Force; throw }
    } else {
      Copy-Item -LiteralPath $tmp -Destination $env:GO_SELF -Force
    }
    Remove-Item -LiteralPath $tmp -ErrorAction SilentlyContinue
    Write-Log "Updated to v$($u.Version) - restarting..."
    $script:Restarting = $true
    $script:Window.Close()
  } catch {
    Write-Log "Update failed: $($_.Exception.Message). Nothing was changed."
  } finally { $script:Window.Cursor = $null }
}

$updateBtn.Add_Click({ Install-Update })
$script:Window.FindName('CheckUpdBtn').Add_Click({ Find-Update $true })
$script:Window.FindName('AutoUpdBtn').Add_Click({ $script:AutoUpdate = -not $script:AutoUpdate; Save-Config; Update-AboutText })
Update-AboutText

# --- Tray icon ---
$script:Tray = New-Object System.Windows.Forms.NotifyIcon
$script:Tray.Text = 'Project OptM'
$script:Tray.Visible = $true
$menu = New-Object System.Windows.Forms.ContextMenuStrip
[void]$menu.Items.Add('Open', $null, { Show-Main })
[void]$menu.Items.Add('Panic: undo everything now', $null, { Invoke-Panic })
[void]$menu.Items.Add('Exit', $null, { $script:Window.Close() })
$script:Tray.ContextMenuStrip = $menu
$script:Tray.Add_DoubleClick({ Show-Main })

# --- Window events ---
$script:Window.Add_SourceInitialized({
  try { [GameOptMem]::DarkTitleBar((New-Object System.Windows.Interop.WindowInteropHelper $script:Window).Handle) } catch {}
})
$script:Window.Add_Activated({
  if (((Get-Date) - $script:LastCheck).TotalSeconds -ge 10) { Update-Checks }
})
$script:Window.Add_StateChanged({
  if ($script:Window.WindowState -eq 'Minimized') {
    $script:Window.Hide()
    if (-not $script:TrayTipShown) {
      $script:Tray.ShowBalloonTip(2500, 'Project OptM', 'Still running here. Double-click the icon to open.', 'Info')
      $script:TrayTipShown = $true
    }
  }
})
$script:Window.Add_Closed({
  try { [OptmHotkey]::Unregister() } catch {}
  $script:Timer.Stop()
  $script:ChartTimer.Stop()
  Restore-All
  $script:Tray.Visible = $false
  $script:Tray.Dispose()
  if ($script:Restarting) {
    try { $script:Mutex.ReleaseMutex() } catch {}
    $env:OPTM_RESTART = '1'
    Start-Process -FilePath $env:GO_SELF
  }
})

# --- Timer (checks every few seconds) ---
$script:Timer = New-Object System.Windows.Threading.DispatcherTimer
$script:Timer.Interval = [TimeSpan]::FromSeconds($PollSeconds)
$script:Timer.Add_Tick({ try { Invoke-Tick } catch { Write-Log "Error: $($_.Exception.Message)" } })
$script:ChartTimer = New-Object System.Windows.Threading.DispatcherTimer
$script:ChartTimer.Interval = [TimeSpan]::FromMilliseconds(250)
$script:ChartTimer.Add_Tick({
  try { if ([OptmHotkey]::Pressed) { [OptmHotkey]::Pressed = $false; Invoke-Panic } } catch {}
  try { Update-Chart } catch {}
})
try {
  if (-not ('OptmHotkey' -as [type])) {
    Add-Type -ReferencedAssemblies System.Windows.Forms -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
public class OptmHotkey : NativeWindow {
  [DllImport("user32.dll")] static extern bool RegisterHotKey(IntPtr h, int id, uint mods, uint vk);
  [DllImport("user32.dll")] static extern bool UnregisterHotKey(IntPtr h, int id);
  public static volatile bool Pressed;
  static OptmHotkey inst;
  public static bool Register(uint mods, uint vk) {
    if (inst == null) { inst = new OptmHotkey(); inst.CreateHandle(new CreateParams()); }
    return RegisterHotKey(inst.Handle, 0x4F50, mods, vk);
  }
  public static void Unregister() {
    if (inst != null) { UnregisterHotKey(inst.Handle, 0x4F50); inst.DestroyHandle(); inst = null; }
  }
  protected override void WndProc(ref Message m) { if (m.Msg == 0x0312) Pressed = true; base.WndProc(ref m); }
}
'@
  }
  if ($script:Settings.PanicHotkey) {
    # MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_END
    if ([OptmHotkey]::Register(0x4003, 0x23)) { Write-Log 'Panic hotkey ready: Ctrl+Alt+End undoes everything instantly' }
    else { Write-Log 'Panic hotkey Ctrl+Alt+End is used by another app - use the tray menu instead' }
  }
} catch { Write-Log "Panic hotkey unavailable: $($_.Exception.Message)" }
Set-Theme
Update-FpsButton

Write-Log "Detected: $cpuName, $($Sys.GPU -replace '  \|  ', ' '), $ramGB GB RAM"
if ($script:CanPin) { Write-Log "Games will be pinned to $($script:BestLabel) (CPU $(Format-Mask ([CpuTopo]::BestMask)))" }
else { Write-Log "CPU layout: $topoText - core pinning not needed" }
$warnCount = @($script:Checks | Where-Object { $_.State -eq 'warn' }).Count
if ($warnCount -gt 0) { Write-Log "$warnCount system check(s) need attention - click the outlined chips to fix" }
if ($script:RestorePlan) { Restore-PowerPlan; Write-Log 'Restored your power plan from last session' }
if ($script:PendingSvcs.Count -gt 0) {
  $script:StoppedSvcs = @($script:PendingSvcs); Resume-Updates
  Write-Log "Restarted services left paused by the last session: $($script:PendingSvcs -join ', ')"
}
if ($script:ProfilesUpgraded) { Write-Log 'Profiles upgraded to v2 - your old file was saved as profiles.old.ini' }
if ($script:GamesAdded -gt 0) { Write-Log "Added $($script:GamesAdded) new games to your profiles" }
if ($script:ImportedRows -gt 0) { Write-Log "Imported $($script:ImportedRows) past sessions into your play history" }
if ($script:OptimizerImported -gt 0) { Write-Log "Imported $($script:OptimizerImported) sessions from Optimizer's history - playtime is on your game tiles" }
Update-VendorPanel
Write-Log "Hardware: $cpuVendor CPU + $(($gpuVendors | Where-Object { $_ -ne 'Other' }) -join ' + ') graphics - maker-specific tweaks on"
Sync-LaunchPriority
Write-Log "$($Profiles.Count) game profiles loaded. Watching for games..."
foreach ($w in $script:ProfileWarnings) { Write-Log "  ! $w" }
Invoke-Tick
$script:Timer.Start()
$script:ChartTimer.Start()
[void]$script:Window.ShowDialog()

try { $script:Mutex.ReleaseMutex() } catch {}

} catch {
  try { Restore-All } catch {}
  [void][System.Windows.MessageBox]::Show("Project OptM hit an error:`n`n$($_.Exception.Message)", 'Project OptM')
}
