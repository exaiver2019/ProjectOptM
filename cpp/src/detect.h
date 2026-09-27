// Finds games that don't have a profile yet, as they start.
// A process counts as a game when it lives in a game library (Steam, Epic, GOG, Ubisoft,
// Xbox, EA, Riot) or Windows' own game list (Game Bar's GameConfigStore), and it opens a
// game-sized window. Launchers, crash reporters, installers and helpers are skipped.
#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <windows.h>
#include "data.h"
#include "processes.h"

struct DetectedGame {
    std::string name;          // "Resident Evil 2"
    std::string exe;           // "re2" (no .exe)
    std::string source;        // "Steam" / "Epic" / ... / "Windows game list"
    std::string keep;          // launcher group to keep open ("steam"), may be empty
    bool antiCheat = false;    // EasyAntiCheat / BattlEye / ... found: safe mode
    std::string antiCheatName;
    std::wstring path;
};

class GameDetector {
public:
    void Load();                                     // reads the libraries and Windows' game list
    // New games among the running processes (each process is only looked at until it's decided)
    std::vector<DetectedGame> Scan(const ProcessList& procs, const AppData& data);

    // Name / store / anti-cheat for an exe in a library (no window check) - also used by tests
    bool Describe(const std::wstring& path, DetectedGame& out) const;
    // Pure helpers, exposed for tests
    static bool LooksLikeHelper(const std::string& exeLower);
    // Anti-cheat files next to any exe (for games added by hand / "Is this a game?")
    bool AntiCheatNear(const std::wstring& exePath, std::string& which) const;
    static std::string GameNameFromVersionInfo(const std::wstring& exePath);

private:
    struct Library { std::wstring root; std::string source, keep; std::string name; };   // name set for per-game roots
    bool Evaluate(const std::wstring& path, DWORD pid, DetectedGame& out);
    const Library* LibraryOf(const std::wstring& lowerPath) const;
    std::string SteamName(const std::wstring& lowerPath) const;
    bool FindAntiCheat(const std::wstring& gameRoot, const std::wstring& exeDir, std::string& which) const;

    std::vector<Library> libs_;
    std::set<std::wstring> windowsGames_;           // GameConfigStore exe paths (lower-case)
    std::map<std::wstring, std::string> steamNames_;   // lower-case steamapps\common\<dir> -> store name
    uint64_t loadedAt_ = 0;
    std::map<ProcKey, uint64_t> pending_;           // looks like a game, waiting for its window (first seen)
    std::set<ProcKey> decided_;
};
