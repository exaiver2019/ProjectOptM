# Project OptM - C++ edition (v2.1.1 experimental)

Native rewrite of Project OptM: Win32 + Direct3D 11 + Dear ImGui.
One self-contained `ProjectOptM.exe` (about 1.2 MB) - no .NET, PowerShell, runtime or PresentMon needed.

## Status

| Phase | What | State |
|---|---|---|
| 1 | Window, sidebar and pages, theme, hardware detection, reads profiles/history/settings, live game detection | Done |
| 2 | Optimization engine (priority, core pinning, background apps, services, power plan, RAM cleanup, launch priority, GPU lock, EcoQoS), tray icon, panic hotkey, health checks with one-click fixes | Done |
| 3 | FPS graph captured directly from Windows (ETW present events - no PresentMon), session history per game with FPS trend and comparisons | Done |
| 4 | Self-updater (GitHub releases, SHA-256 checked), release tooling | Done |

It replaces Project OptM 1.1 and uses the same files in `%APPDATA%\ProjectOptM`, so profiles, launch
shortcuts, history and theme carry over (and you can switch back and forth). The two share a
single-instance lock, so only one of them runs at a time.

## Building

Double-click **Build.bat**. It finds Visual Studio 2022 and writes `dist\ProjectOptM.exe`.

You need Visual Studio 2022 (Community is free) or just its Build Tools, with "Desktop development with C++":
```
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Or by hand:
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**Linux / WSL** (MinGW-w64): `./build-mingw.sh`

## Release channels

`OPTM_CHANNEL` in `src/version.h` is one line that says which build this is and who it's for. The
sidebar and About page show it automatically (colored, with a tooltip) - nothing else needs editing.

| Channel | `OPTM_CHANNEL` | Badge | Who it's for |
|---|---|---|---|
| **Stable** | `""` | none | Anyone, including friends. The only channel published to GitHub. |
| **Experimental** | `"experimental"` | amber | You (or anyone who wants new things early). A local build ahead of the next release - its features have each been tried end to end, but not everything is proven on real hardware yet. |
| **Unstable** | `"unstable"` | red | You only, only while actively working on it with Claude. Something in it may be untested, broken, or mid-edit. Never shared, and not for a game session you care about. |

A build only moves stable -> released once every feature in it has been tested (see each feature's
notes as it's added, and the sandbox testing rules below). Before that it's experimental; while it's
being changed and something in it hasn't been tried yet, call it unstable.

## Releasing

1. Set `OPTM_CHANNEL` to `""` in `src/version.h` (see above) and raise `OPTM_VERSION` / `OPTM_VERSION_RC`.
2. Commit and push (the front page is the `README.md` at the repo root).
3. Double-click **Publish-Release.bat**. It builds, asks what changed, and publishes a GitHub release
   with `ProjectOptM.exe`, titled "Project OptM vX.Y.Z (Stable)" and marked latest.

**Experimental pre-releases.** With `OPTM_CHANNEL "experimental"` and `OPTM_PRERELEASE` set to 1 (then 2, 3...
for the next ones of the same version), Publish-Release.bat publishes tag `vX.Y.Z-experimental.N` as a GitHub
*pre-release*. Only people who chose Settings > Update channel: **Experimental** are offered it; stable users
(and every older version of the app, which only asks for `releases/latest`) never see it. Unstable builds are
refused. The app orders versions as numbers first, then stable > experimental > unstable, then N - so
`2.1.1` (stable) replaces `2.1.1-experimental.3`, which replaces a local `2.1.1` experimental build (N = 0).

Everyone on 1.1 or later gets the update from inside the app within a few hours. The updater
checks the SHA-256 GitHub publishes for the file, keeps the previous exe in `%APPDATA%\ProjectOptM\backup`,
and restarts into the new version.

## Layout

```
src/main.cpp          window, Direct3D 11, render loop (idles near 0% CPU), tray/hotkey messages, restart
src/app.cpp/.h        sidebar, pages, theme, tray icon, session history UI
src/optimizer.*       the engine: detects games, applies profiles, puts everything back
src/processes.*       process list + per-process tweaks (priority, affinity, EcoQoS, closing, standby purge)
src/tweaks.*          services, power plans, launch priority (IFEO), GPU preference, session tweaks
src/tweakset.*        the Tweaks page catalog: every tweak, its details, presets, per-game tweaks
src/detect.*          finds new games (game libraries, Windows' game list, window size)
src/checks.*          health checks and their one-click fixes
src/frames.*          FPS capture (ETW: Microsoft-Windows-DXGI + D3D9 present events), FPS statistics, stutters
src/insights.*        game tests (CCD, tweak A/B), session comparison, crash patterns, heat, FPS cap advice - pure functions
src/screens.*         screen refresh rates; a video / stream playing on another screen (audio meters + window positions)
src/crashes.*         crash detection: exit codes + Windows Error Reporting events (Application log 1000/1002)
src/latency.*         stutter-cause finder: kernel DPC/ISR trace (system logger session), matched against stutters
src/netping.*         server ping: Kernel-Network ETW finds the game's server, ICMP pings it
src/driverinfo.*      graphics driver version, shader cache folders/size, AMD Adrenalin settings (read-only)
src/share.*           game profile share codes (OPTM-GAME1: + base64 JSON)
src/updater.*         GitHub release check, download, verify, swap
src/system_info.*     CPU topology, RAM (SMBIOS), GPUs (DXGI), display, Windows version
src/data.*            profiles.ini, settings.json, history.csv (same formats as 1.1)
src/json.*, util.*    helpers
src/version.h         version number and update repo
res/                  icon, manifest (asInvoker - main.cpp asks for admin itself), version info
third_party/imgui     Dear ImGui 1.91.9 (MIT license)
```

## Tweaks

The Tweaks page (Ctrl+5) lists all 39 tweaks with an on/off switch, grouped into CPU, Memory, System,
GPU, Power and Input, with search (Ctrl+F) and category filters.

- **Presets:** Safe (what 1.x always did, plus Game Mode on, pausing Automatic Maintenance and keeping the
  screen awake - the default), Balanced (adds timer resolution, I/O priority, MMCSS, explorer/audio/DWM
  priority, animations and transparency off, Game DVR capture off, fullscreen optimizations off, PCIe link
  power saving off, the Sticky/Filter/Toggle Keys shortcuts off, working-set trim) and Aggressive (adds SMT
  scheduling, core unparking, C-state disable, max boost). Mouse acceleration off is opt-in (it changes aim feel).
- **Only tweaks that do something:** network "throttling", TCP/Nagle, DNS flushing, dynamic tick, HPET and
  similar placebo tweaks are left out on purpose, and so are ones Windows no longer honours (the global
  background-apps switch) or that need Explorer restarted (the notifications switch - Windows 11 turns on
  Do not disturb for full-screen games by itself).
  Built-ins are locked; flipping a switch makes an editable copy. Your presets can be renamed, duplicated,
  exported to / imported from a `.json` file and deleted.
- **Everything is per session:** tweaks are applied when a game starts and undone when it closes.
  Changes apply to the next game. System values are backed up in settings.json first, so if the app
  crashes mid-game the next start puts them back.
- **Hardware-aware:** tweaks that don't fit the PC are locked with the reason - core unparking,
  C-state disable and max boost are off on X3D chips because they fight AMD's V-Cache core parking.
- Fullscreen optimizations off is the one lasting change (Windows reads it at launch); turning the
  tweak off removes it again for every game it was set on.
- Defender exclusion is in no preset - it's there if you want it, with a warning.
- Click a tweak (or turn on "Show all details") to see what it does, its pros and its cons.

## New-game detection

Games without a profile are added automatically when they start (Settings > Detect new games).
A process counts as a game when:
1. it lives in a game library - every Steam library, Epic, GOG, Ubisoft Connect, the Xbox app's
   XboxGames folders, EA and Riot - or in Windows' own game list (Game Bar's GameConfigStore), and
2. it opens a game-sized window (800x600 or bigger, or fullscreen) within 90 seconds, and
3. its name doesn't look like a helper (launchers, crash reporters, installers, anti-cheat wrappers).

The profile gets the store's name for the game (Steam app manifest, Epic/GOG data, the exe's version
info or its folder), keeps its launcher open, and is optimized right away. If EasyAntiCheat, BattlEye,
EA Javelin or similar is found in the game folder (or running), it gets anti-cheat safe mode.
New games show a NEW tag and an ✕ "Not a game" button, which removes the block from profiles.ini and
adds the exe to `IgnoredExes` in settings.json. `OPTM_TEST_LIBRARY=<folder>` adds a test library.

## Game settings

The gear on each game's tile opens its settings, so nothing has to be edited by hand:
exe names, anti-cheat safe mode, launcher, priority, cores (locked / preferred / other CCD / all),
efficiency-mode blocking, launch priority, RAM cleanup, boost / close / keep-open apps, and **tweaks**.
A game can follow the Tweaks page preset (the default), use another preset, or have its own custom set
of tweaks (`tweaks = Balanced` or `tweaks = custom: timer, fso` in profiles.ini). Saving rewrites only
that game's block - notes and alignment are kept - and re-applies straight away if the game is running.
Renaming a preset on the Tweaks page updates the games that use it; a deleted preset falls back to the
Tweaks page preset. "Remove game" deletes the profile (play history is kept).

## Exiting puts everything back

Closing the window or tray > Exit undoes the running session (priorities, services, power plan, tweaks)
and, with Settings > **Restore everything on exit** (on by default), also the per-game settings Windows
applies at launch: launch priority (IFEO), fullscreen optimizations and the dedicated-GPU preference -
your own previous GPU preference is recorded (`GpuManaged` in settings.json) and put back. They're set
again when the app starts. Restarting into a new build doesn't revert them.

## Taskbar, minimize and the jump list

Minimize is a normal minimize: the window stays on the taskbar and in Alt+Tab (it used to hide in the tray).
Right-click the taskbar button for **Hide to tray** and **Overlay on / off**.

How it works without a UAC prompt per click: the exe's manifest is `asInvoker`, and main.cpp asks for admin
itself - opened normally, it relaunches with `runas` (the usual prompt, every time the app is opened). The
jump-list items run the exe with `--cmd hide` / `--cmd overlay`; that copy never elevates, it just posts the
registered message `ProjectOptM.Command` to the running window and exits. The running (admin) window lets that
one message through with `ChangeWindowMessageFilterEx` - it can only show or hide the window, or flip the
overlay. Opening the app while it's already running brings the window up the same way (no second prompt).

## In-game overlay

The **Overlay** page (or Ctrl+Alt+O) shows FPS and any of: 1% low,
frametime, a mini frametime graph (last 4 s, spikes in amber), GPU usage + temperature, VRAM, CPU usage, RAM.
**Update speed**: 2 / 4 / 10 / 20 redraws a second (default 10, about 1% of one core; 20 is about 3%). The main
loop wakes for each redraw even from the tray (App::MaxWaitMs) without redrawing the app window itself. Its spot is set in the page's **Position** card: a picture of your screen (its real shape,
the game's monitor while one runs) with the overlay drawn at its real size - drag it, click anywhere to move it
there, or snap to a corner. Saved as 0..1 across the screen, so it lands in the same place on any monitor. Background opacity 0-100%
(0 = outlined text only) and size 70-160%.
It's a GDI+ layered window with per-pixel alpha (`UpdateLayeredWindow`) - `WS_EX_TOPMOST | WS_EX_LAYERED |
WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW`, re-asserted topmost every second, so it never takes
focus or clicks and isn't in Alt+Tab.
Readings (src/sensors.*, a background thread, once a second, only while shown): GPU usage and VRAM from the
`GPU Engine` / `GPU Adapter Memory` performance counters for the main GPU's LUID (Task Manager's method), GPU
temperature from `D3DKMTQueryAdapterInfo(ADAPTERPERFDATA)` when the driver reports it, CPU from
`% Processor Utility`, RAM from `GlobalMemoryStatusEx`.
Nothing is injected into the game (unlike RTSS/Afterburner, which hook Present - anti-cheat risk). The
trade-off: it only shows over windowed, borderless and flip-model "fullscreen" games, not true exclusive
fullscreen. It shows only while the game window is in front, hides when FPS capture is blocked, and by
default stays off over anti-cheat games (Overlay page > Anti-cheat games). (src/overlay.*)

## Start with Windows

Settings > **Start with Windows** creates a Task Scheduler task, "Project OptM (<user>)": at sign-in (10 s
delay) it runs the exe with `--tray` and *Run with highest privileges*, so the app starts hidden in the tray
already elevated - no UAC prompt. Normal priority, no time limit, runs on battery. The task can't be
started on demand (`AllowStartOnDemand` is false), so no other program can use it to start the app as admin;
opening the app by hand always shows the normal UAC prompt. Turning it off deletes the task. If the exe moves, the next start points the task at the new path. Test copies (`--data-dir`) never
touch the task. A self-restart after an update comes back in the tray if the window was hidden.
(src/autostart.*)

## Feedback

**Send feedback** (bottom of the sidebar, or the tray menu) opens a form: type (Bug / Idea / Game
request / Other), title, details and an optional game, plus switches for what to attach - PC specs, that
game's settings, its last session's FPS, and the last 100 activity lines. A preview shows the exact text.
It becomes a GitHub issue on `OPTM_UPDATE_REPO`: the app opens a filled-in "new issue" page and the user
clicks Submit there, so nothing is sent from the app and no token is in the exe. Attachments sit in
collapsed `<details>` blocks. `C:\Users\<name>` in any path becomes `C:\Users\<you>`. Reports too long
for a link (over ~4000 characters - GitHub's sign-in page wraps the link again) go on the clipboard, and
the issue page says to paste them. **Copy** puts the report on the clipboard for Discord, email and so on.

## Intel and laptops

- **Soft core pinning** (tweak, or `cores = Prefer` per game): games are steered to the P-cores with
  Windows CPU sets instead of locked to them, so games that use many threads can still use the E-cores.
  Forced on when **Intel APO** is installed, so the two don't fight.
- **Background efficiency mode** (Balanced): background apps run in Windows efficiency mode, which
  Intel Thread Director keeps on the E-cores.
- **Health checks:** 13th/14th gen microcode (0x12B minimum, 0x12F latest), Core Ultra 200S
  performance fixes (microcode 0x114+ and Windows 11 24H2), Intel APO detection.
- **Laptops:** a warning when a game starts on battery, and a **Best performance power mode** tweak
  (Balanced; not on X3D chips).
- The Intel rules are covered by a test program with real CPU names (they can't run on an AMD PC).
- Settings in profiles.ini still apply: a feature turned off there shows as locked here.

## Interface size

The UI follows Windows' display scaling. On big screens left at 100% scaling (1440p and up) it
starts at 125% (150% on 4K) so it isn't tiny. Settings > Appearance > Interface size, or Ctrl + / Ctrl - /
Ctrl 0, changes it; the choice is saved as `UiScale` in settings.json (0 = automatic).

## FPS capture notes

Frame times come from the present events Direct3D 9/10/11/12 raise (the same events PresentMon reads),
so they cover DirectX games. OpenGL and Vulkan games that don't present through DXGI show no FPS - the
optimizing itself works the same for every game. Capturing needs admin rights, which the app already has.

## 2.1.1 experimental features

- **0.1% lows and stutters.** A stutter is a frame over 2.5x the median of the last 90 frames and at least
  10 ms more. Home shows the session's 0.1% low and stutter count, the graph marks stutters red, and each
  session keeps them. The overlay can show Stutters (per minute) and Ping; its graph draws stutters red.
- **Session details** (`session-details.json`, keyed `"<date>|<game>"`): 0.1% low, stutters, seconds with FPS,
  how it ended, tweak preset + ids, cores, test variant, GPU temp avg/max, CPU use, ping, stutter cause.
  `history.csv` is unchanged, so 1.x still reads it. Crashed sessions are kept even when under a minute.
- **Loading vs gameplay stutters**: every 250 ms the game's own read counter (query rights, like Task Manager)
  is checked; 20 MB/s or more marks it loading, and it stays marked 2 s after. Stutters then count as
  `LoadStutters`, the seconds as `LoadSeconds`, and those frames are left out of the 1% / 0.1% / 5% lows.
  Stutters per minute use gameplay only. A game that "loads" over half the session just streams while you
  play, so then everything counts again.
- **Compare** (history window tab): two ticked sessions (or the last two) side by side, and which tweaks differed.
- **Tests** (history window tab): `ccd` (V-Cache vs frequency CCD, 2 sessions each, dual-CCD X3D only, not for
  anti-cheat games) or `ab:<tweak>` (on vs off, 3 each). Stored in settings.json `Experimental.Tests` as
  `"<id>|<start date>"`; the optimizer's `onPrepare` hook swaps the session's cores / tweak set, the profile isn't
  touched. A session counts at 5+ minutes with FPS. 1% lows decide, under 3% is "no clear difference". Applying
  the result is a button.
- **Crash detection**: exit code >= 0xC0000000 (query-only handle, never for anti-cheat games) or an
  Application Error (1000) / Application Hang (1002) event naming the exe within the session. The history
  window shows crash rates per tweak preset once there are 2+ crashes.
- **Is this a game?** (Settings > Experimental, on by default): an app without a profile that covers its whole
  monitor in the foreground for 2 minutes is asked about on Home (Yes adds it - anti-cheat folders next to the
  exe turn on safe mode; No adds it to IgnoredExes; Not now skips it until restart). Browsers, players,
  launchers and Windows' own apps are never asked about.
- **Pause cloud sync** tweak (`cloud_sync`, opt-in): closes OneDrive / Dropbox / Google Drive / MEGA at game
  start; each exe is saved as an `O|<path>` backup, so it's opened again at the end or on the next start after
  a crash. Test copies only close `OPTM_TEST_CLOUD` stand-ins.
- **Stutter-cause finder** (Settings > Experimental, off by default): a system-logger kernel session with the
  DPC + INTERRUPT flags; DPCs/ISRs of 100 us+ are kept for 10 s, and each stutter blames the longest one
  (0.5 ms+) that overlaps it. Drivers are named with EnumDeviceDrivers. Needs admin.
- **Server ping** (Settings > Experimental, off by default): Microsoft-Windows-Kernel-Network send events
  (10/26 TCP, 42/58 UDP) for the game's PIDs pick the address with the most UDP bytes (else non-web TCP);
  it's pinged every 2 s. Needs admin; some servers don't answer pings.
- **Graphics driver** card (System): driver version (a change since last start suggests clearing the shader
  cache), shader cache size, and AMD's global Chill / Anti-Lag / Boost / VSync values from the display class key.
- **Share codes**: game settings > Copy share code; Games > Add from code shows what a code adds first.
- **Timeline** (Activity): every session's changes and their undo, crashes, tests, fixes and Windows settings
  (`timeline.json`, last 400), plus what's still changed on the PC with a button to put it back now.
- **Heat warnings**: one sample a second (GPU temp, GPU use, FPS). Sessions keep the seconds at 85 C+; a likely
  throttle is FPS 10%+ lower while hot than while under 80 C, both at 90%+ GPU use (so a CPU-bound game or a
  quieter scene doesn't count). A whole minute at 90 C+ is logged live. (GPU sensor only - Windows doesn't
  report CPU temperature without a driver.)
- **FPS cap advice** (history window, summary): FPS above the refresh rate -> cap at refresh - 3 (keeps VRR in
  range); big swings (5% low under 75% of the average) -> a cap near the 5% low, only if it keeps 60%+ of the
  average. Where to set it depends on the GPU maker. Sessions keep `Hz` (the game's screen) and `Fps5`.
- **Video on the other screen**: every 5 s during a session, a browser or video player that is making sound
  (Windows' per-app audio meters) with a window on another screen is counted (`OtherVideoSeconds`). A health
  check says so when your screens run at different refresh rates.
- **Session summary** (Settings > Test features, on by default): a notification when a game closes - FPS,
  lows, stutters, vs last time, and heat / video / cap notes. A click opens that game's history.
- **Update channel** (Settings > Updates): Stable (`releases/latest`) or Experimental (the newest of the last
  20 releases, pre-releases included; drafts and unstable tags are skipped). Defaults to the running build's
  channel. Unstable builds never auto-check.
- **Backup and restore** (Settings > Profiles and data): one `.optm` file (JSON) with profiles.ini,
  settings.json, history.csv, session-details.json and timeline.json. This PC's state (crash-recovery backups,
  launch priority / GPU / FSO records, paused services, power plan, driver version) is never exported, and on
  restore the current PC's values are kept. The files being replaced go to `backup-before-restore-<time>`.

## Developer switches

Handy for testing without touching your real setup:

| Switch | What it does |
|---|---|
| `--page home\|games\|sessions\|overlay\|tweaks\|system\|activity\|settings\|about` | open on that page |
| `--how-it-works` | open the About page's How it works window |
| `--intro` (with `--screenshot`) | play the opening intro in a screenshot run (normally skipped there) |
| `--data-dir <folder>` (or `OPTM_DATA_DIR`) | use another data folder instead of `%APPDATA%\ProjectOptM` |
| `--log-file <file>` (or `OPTM_LOG_FILE`) | copy the activity log to a file |
| `--screenshot <file.png>` `--shot-delay <ms>` `--exit-after <ms>` | save the window to a PNG, then exit |
| `--fps-self` | graph the app's own frames (checks FPS capture without a game) |
| `--zoom-after <percent>` | change the interface size 1 s after start (checks live resizing) |
| `--history <game>` | open that game's history window |
| `--game-settings <game>` (`--save-settings`) | open that game's settings (and save them at once - a profiles.ini round-trip check) |
| `--tweak-roundtrip` (test copies only) | apply the invisible session tweaks (mouse acceleration, Sticky Keys shortcuts, Game Mode, PCIe power saving) for a moment, undo them, and log every value |
| `--overlay-demo` (with `--fps-self`) | show the in-game overlay over the app's own window (if it's on in settings) |
| `OPTM_TEST_SYSTEM=1` | let a test copy change Windows-wide settings (launch priority, GPU, FSO) - use only with made-up test games |
| `--feedback` (`--feedback-preview`) | open the feedback form (with its preview) |
| `OPTM_FEEDBACK_TEST=<title>` | build a feedback link at start and log it instead of opening it (`--feedback-long` tests the clipboard path) |
| `OPTM_UPDATE_AS=<version>` | pretend to be an older version when checking for updates |
| `--synthetic-fps` (test copies only) | made-up frames at ~140 FPS with regular 45/90 ms stutters while a game session runs - FPS screens without admin |
| `--history-tab 0\|1\|2` (with `--history`) | open the history window on Sessions / Compare / Tests |
| `--share-code <game>` | log that game's share code and its decoded settings (round-trip check) |
| `--import-code <code>` / `--import-add <code>` (test copies) | open Add from code with a code / add the game at once |
| `--ask-game <exe path>` (`--ask-answer 1\|0\|-1`, test copies) | show the "Is this a game?" card for an exe (and answer it) |
| `OPTM_TEST_FOREGROUND=<window class>` + `OPTM_TEST_ASK_SECS=<s>` | test copies: treat that (hidden) window as the foreground app, and ask after that many seconds |
| `OPTM_TEST_CLOUD=<exe names>` | test copies: the "cloud apps" the Pause cloud sync tweak closes (stand-ins) |
| `--insights-selftest 1` (test copies) | log the heat and FPS-cap results for made-up cases |
| `--summary-last <game>` (test copies) | show the session summary for that game's latest session (and log its text) |
| `--backup-to <file>` / `--restore-from <file>` (test copies) | back up / restore without the file dialogs |
| `--check-updates stable\|experimental` (test copies) | check for updates on that channel at start, logging the result |
| `OPTM_TEST_UPDATE_BASE=<url>` | test copies: ask this server instead of api.github.com (e.g. `http://127.0.0.1:8765/repos/test`) |
| `OPTM_UPDATE_AS=<version>` | also takes channel tags, e.g. `2.1.1-experimental.2` |
| `OPTM_TEST_GAME_WINDOW=<window class>` | test copies: the window treated as the game's (second-screen video check) |

A test copy (`--data-dir`) runs without admin and never asks for it (anything needing admin then fails and is logged).
