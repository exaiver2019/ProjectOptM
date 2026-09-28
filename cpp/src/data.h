// Project OptM's shared data files in %APPDATA%\ProjectOptM
// (the same files the 1.x app uses, so everything carries over).
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct GameProfile {
    std::string name;
    std::vector<std::string> exes;
    bool antiCheat = false;
    std::string priority = "AboveNormal";   // Normal / AboveNormal / High
    std::string cores = "Best";             // Best / Other / All
    bool softPin = false;                   // cores = Prefer: Best, but steered with CPU sets instead of locked
    int ramCleanupMins = 0;
    std::vector<std::string> boost, keep, close;
    std::string launchPriority;             // empty = off
    bool ecoQosOff = true;
    std::string tweaks;                     // "" = the Tweaks page preset, a preset name, or "Custom"
    std::vector<std::string> tweakIds;      // tweaks = Custom: the tweaks that are on for this game
};

std::string PriorityLabel(const std::string& priority);   // "AboveNormal" -> "Above normal", "" -> "Off"

// [Settings] block of profiles.ini
struct IniSettings {
    int poll = 3;                                       // seconds between game checks
    bool pauseUpdates = true;
    std::string powerPlan = "auto";                     // auto / high / off
    bool forceGpu = true;
    bool cleanupOnLaunch = true;
    std::vector<std::string> close;
    std::vector<std::string> pauseServices = { "SysMain", "WSearch" };
    bool panicHotkey = true;
    bool reopenClosed = true;
    std::string backgroundCores = "auto";               // auto / off
    bool vendorApps = true;
    std::vector<std::string> background;
};

struct Session {
    std::string date, game;                // date: "yyyy-MM-dd HH:mm"
    double minutes = 0, avgFps = 0, low1 = 0;
    std::string version;                   // app version that recorded it ("imported" / "Optimizer" for imports)
    // 2.1.1 details (session-details.json, so history.csv stays as the 1.x app reads it)
    double low01 = 0;                      // 0.1% low FPS
    int stutters = -1;                     // frames far slower than the ones around them (-1 = not recorded)
    double fpsSeconds = 0;                 // how much of the session had FPS
    std::string exit;                      // "" = closed normally / unknown, "crash", "crash 0xC0000005", "hang"
    std::string preset;                    // tweak preset used ("Safe", "Custom", ...)
    std::string cores;                     // "Best" / "Other" / "All" / "Prefer" / "safe mode"
    std::string variant;                   // test build-up: "ccd:vcache", "ab:timer:on", ...
    double gpuTempAvg = -1, gpuTempMax = -1, cpuAvg = -1, pingAvg = -1;
    std::string cause;                     // stutter-cause finder: the driver behind most stutters
    std::vector<std::string> tweaks;       // tweak ids that were on
    int hz = 0;                            // refresh rate of the screen the game was on
    double fps5 = 0;                       // 5% low FPS (95% of frames were faster) - for FPS cap advice
    int hotSeconds = 0;                    // seconds the GPU was at 85 C or more
    double heatDrop = 0;                   // % lower FPS while hot and fully loaded (0 = no sign of throttling)
    int otherVideoSeconds = 0;             // seconds a video / stream played on another screen
    double StuttersPerMin() const { return stutters >= 0 && fpsSeconds >= 30 ? stutters * 60.0 / fpsSeconds : -1; }
};

struct Theme {
    std::string accent = "#4F8BFF";
    std::string background = "Dark";       // Dark / Darker / OLED Black / Slate
    std::string corners = "Rounded";       // Rounded / Soft / Sharp
};

struct AppData {
    // profiles.ini
    std::vector<GameProfile> profiles;
    IniSettings settings;
    std::vector<std::string> warnings;
    int64_t profilesStamp = 0;                         // last write time, for live reload
    bool profilesCreated = false;                      // written from the defaults on this start

    // settings.json
    std::map<std::string, std::string> launchPaths;   // game name -> shortcut/exe
    Theme theme;
    bool autoOptimize = true;
    bool fpsOn = true;
    bool autoUpdate = true;
    double uiScale = 0;                                // interface size (1.25 = 125%); 0 = automatic
    std::string tweakPreset = "Safe";                  // active Tweaks preset (built-in or user)
    std::map<std::string, std::vector<std::string>> tweakPresets;   // user presets: name -> tweak ids that are on
    std::vector<std::string> tweakBackups;             // system values to put back if we crash mid-game
    std::vector<std::string> fsoManaged;               // game exes we turned fullscreen optimizations off for
    bool autoDetect = true;                            // add games that have no profile when they start
    bool animations = true;                            // subtle UI animations
    bool intro = true;                                 // short animated greeting when the app opens
    std::string greetName;                             // what the greeting calls you ("" = no name)
    bool tourDone = false;                             // the welcome tour has been shown
    std::vector<std::string> ignoredExes;              // "Not a game" - never detect these again
    std::vector<std::string> autoAdded;                // profile names that detection added
    std::vector<std::string> ifeoManaged;              // exes we set a launch priority for
    std::vector<std::string> gpuManaged;               // "exe path|value before us" we set the GPU preference for
    bool revertOnExit = true;                          // exiting also removes launch priority / GPU / FSO settings
    // in-game FPS overlay (a click-through window above the game - nothing is loaded into the game)
    bool overlayOn = false;
    double overlayX = 0, overlayY = 0;                 // where it sits: 0 = left/top edge ... 1 = right/bottom edge of the screen
    int overlayOpacity = 85;                           // background, percent (0 = see-through, text only)
    int overlaySize = 100;                             // percent
    int overlayRate = 10;                              // redraws per second (2, 4, 10 or 20)
    std::vector<std::string> overlayItems = { "low", "frametime", "graph" };   // shown under the FPS (see OverlayItems in app.cpp)
    bool overlayAntiCheat = false;                     // also over anti-cheat games (off: their anti-cheat might not like it)
    std::string restorePlan;                           // power plan to go back to (crash recovery)
    std::vector<std::string> pausedServices;           // services to restart (crash recovery)
    std::string optimizerImport;                       // newest Optimizer session already imported
    // 2.1.1 experimental
    std::map<std::string, std::string> tests;          // game -> running test ("ccd", "ab:<tweak id>")
    bool latencyOn = false;                            // stutter-cause finder (driver latency trace while playing)
    bool pingOn = false;                               // measure the game server's ping while playing
    bool askGames = true;                              // "Is this a game?" for full-screen apps without a profile
    std::string gpuDriverSeen;                         // graphics driver version at the last start (shader cache hint)
    std::vector<std::string> timeline;                 // "yyyy-MM-dd HH:mm:ss|kind|text", newest last
    bool summaryOn = true;                             // a notification with the numbers when a game closes
    std::string updateChannel;                         // "stable" / "experimental" ("" = this build's own channel)
    std::string UpdateChannel() const;                 // the one in use

    // Backup / restore: one file with profiles, settings, history, details and timeline.
    // PC-specific state (crash recovery lists, launch priority records...) is left out and never restored.
    bool ExportBackup(const std::wstring& file, std::string& error) const;
    bool ImportBackup(const std::wstring& file, std::string& error);   // the current files are kept in backup-before-restore-<time>

    // history.csv
    std::vector<Session> history;
    std::map<std::string, double> playtime;           // game name -> minutes

    void Load();                   // everything
    bool LoadProfiles();           // profiles.ini only; false if it couldn't be read
    void LoadConfig();
    void LoadHistory();
    void SaveConfig() const;       // settings.json, keeping keys we don't know about
    bool ProfilesChanged() const;  // profiles.ini edited (or deleted) since the last load
    void AddSession(const Session& s, const std::string& version);
    void AddTimeline(const std::string& kind, const std::string& text);   // kept in timeline.json (last 400)
    int ImportOtherHistory();      // history-import.csv + the "Optimizer" app's log; returns sessions added
    // Adds a [name] block to profiles.ini (name made unique); returns the section name used
    std::string AppendProfile(const std::string& name, const std::string& exe, const std::string& source,
                              const std::string& keep, bool antiCheat, const std::string& antiCheatName);
    bool RemoveProfile(const std::string& name);   // deletes its [name] block from profiles.ini
    // Rewrites p's [name] block with its settings, keeping notes and keys the app doesn't manage
    bool SaveProfile(const GameProfile& p);

    std::wstring ProfilesPath() const;
    std::wstring DataDir() const;
    std::wstring HistoryPath() const;
    std::wstring DetailsPath() const;   // session-details.json
};
