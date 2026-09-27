# Project OptM - C++ edition (v2.0)

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

## Releasing

1. Raise `OPTM_VERSION` and `OPTM_VERSION_RC` in `src/version.h`.
2. Commit and push (the front page is the `README.md` at the repo root).
3. Double-click **Publish-Release.bat**. It builds, asks what changed, and publishes a GitHub release
   with `ProjectOptM.exe`.

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
src/frames.*          FPS capture (ETW: Microsoft-Windows-DXGI + D3D9 present events) and FPS statistics
src/updater.*         GitHub release check, download, verify, swap
src/system_info.*     CPU topology, RAM (SMBIOS), GPUs (DXGI), display, Windows version
src/data.*            profiles.ini, settings.json, history.csv (same formats as 1.1)
src/json.*, util.*    helpers
src/version.h         version number and update repo
res/                  icon, manifest (asks for admin), version info
third_party/imgui     Dear ImGui 1.91.9 (MIT license)
```

## Tweaks

The Tweaks page (Ctrl+3) lists every tweak with an on/off switch, grouped into CPU, Memory, System,
GPU and Power, with search (Ctrl+F) and category filters.

- **Presets:** Safe (what 1.x always did - the default), Balanced (adds timer resolution, I/O priority,
  MMCSS, explorer/audio/DWM priority, animations off, Game DVR capture off, fullscreen optimizations off,
  working-set trim) and Aggressive (adds SMT scheduling, core unparking, C-state disable, max boost).
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

## Developer switches

Handy for testing without touching your real setup:

| Switch | What it does |
|---|---|
| `--page home\|games\|system\|activity\|settings` | open on that page |
| `--data-dir <folder>` (or `OPTM_DATA_DIR`) | use another data folder instead of `%APPDATA%\ProjectOptM` |
| `--log-file <file>` (or `OPTM_LOG_FILE`) | copy the activity log to a file |
| `--screenshot <file.png>` `--shot-delay <ms>` `--exit-after <ms>` | save the window to a PNG, then exit |
| `--fps-self` | graph the app's own frames (checks FPS capture without a game) |
| `--zoom-after <percent>` | change the interface size 1 s after start (checks live resizing) |
| `--history <game>` | open that game's history window |
| `--game-settings <game>` (`--save-settings`) | open that game's settings (and save them at once - a profiles.ini round-trip check) |
| `OPTM_UPDATE_AS=<version>` | pretend to be an older version when checking for updates |

Set `__COMPAT_LAYER=RunAsInvoker` to start it without the admin prompt (anything needing admin then fails and is logged).
