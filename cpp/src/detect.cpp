#include "detect.h"
#include "json.h"
#include "util.h"
#include <algorithm>
#include <regex>
#include <shlobj.h>

namespace {

std::wstring LowerW(std::wstring s) { std::transform(s.begin(), s.end(), s.begin(), ::towlower); return s; }
std::wstring WithSlash(std::wstring s) { if (!s.empty() && s.back() != L'\\') s += L'\\'; return s; }
bool IsDir(const std::wstring& p) { DWORD a = GetFileAttributesW(p.c_str()); return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY); }
bool Exists(const std::wstring& p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }
std::wstring Parent(const std::wstring& p) { size_t s = p.find_last_of(L"\\/"); return s == std::wstring::npos ? L"" : p.substr(0, s); }
std::wstring FileName(const std::wstring& p) { size_t s = p.find_last_of(L"\\/"); return s == std::wstring::npos ? p : p.substr(s + 1); }

// "key"  "value" pairs from Valve's text formats (libraryfolders.vdf, appmanifest_*.acf)
std::vector<std::string> VdfValues(const std::string& text, const char* key) {
    std::vector<std::string> out;
    std::regex re(std::string("\"") + key + "\"\\s+\"([^\"]*)\"", std::regex::icase);
    for (auto it = std::sregex_iterator(text.begin(), text.end(), re); it != std::sregex_iterator(); ++it) {
        std::string v = (*it)[1];
        std::string u;
        for (size_t i = 0; i < v.size(); i++) { if (v[i] == '\\' && i + 1 < v.size() && v[i + 1] == '\\') i++; u += v[i]; }
        out.push_back(u);
    }
    return out;
}

// Registry subkeys of root\path, with a string value from each
void EachSubkey(HKEY root, const wchar_t* path, void (*fn)(HKEY, const wchar_t*, void*), void* ctx) {
    HKEY k;
    if (RegOpenKeyExW(root, path, 0, KEY_READ | KEY_WOW64_32KEY, &k) != ERROR_SUCCESS) return;
    wchar_t name[256];
    for (DWORD i = 0;; i++) {
        DWORD len = 256;
        if (RegEnumKeyExW(k, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        fn(k, name, ctx);
    }
    RegCloseKey(k);
}

struct WinCtx { DWORD pid; bool big; };
BOOL CALLBACK BigWindow(HWND w, LPARAM lp) {
    auto* c = (WinCtx*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid != c->pid || !IsWindowVisible(w) || GetWindow(w, GW_OWNER) || IsIconic(w)) return TRUE;
    RECT r;
    if (!GetWindowRect(w, &r)) return TRUE;
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(w, MONITOR_DEFAULTTONEAREST), &mi);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    bool fullscreen = r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top && r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom;
    if (fullscreen || (ww >= 800 && wh >= 600)) { c->big = true; return FALSE; }
    return TRUE;
}

const char* kAntiCheatProcesses[] = { "EasyAntiCheat", "EasyAntiCheat_EOS", "BEService", "BEService_x64", "vgc", "FACEIT", "EAAntiCheat.GameServiceLauncher", "mhyprot3" };

}  // namespace

// ------------------------------------------------------------ libraries
void GameDetector::Load() {
    libs_.clear();
    windowsGames_.clear();
    steamNames_.clear();
    loadedAt_ = GetTickCount64();
    // A test copy (own data folder) only looks at its test library - never at the games you're playing
    if (!util::EnvVar(L"OPTM_DATA_DIR").empty()) {
        std::wstring testLib = util::EnvVar(L"OPTM_TEST_LIBRARY");
        if (!testLib.empty()) libs_.push_back({ LowerW(WithSlash(testLib)), "Test library", "", "" });
        return;
    }

    // Steam: every library folder, plus the store names from the app manifests
    std::string steamPath = util::RegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath");
    std::vector<std::string> steamLibs;
    std::string text;
    if (!steamPath.empty()) {
        steamLibs.push_back(steamPath);
        if (util::ReadFile(util::Widen(steamPath) + L"\\steamapps\\libraryfolders.vdf", text))
            for (auto& p : VdfValues(text, "path")) steamLibs.push_back(p);
    }
    std::set<std::wstring> seenSteam;
    for (auto& lib : steamLibs) {
        std::wstring apps = WithSlash(util::Widen(lib));
        for (auto& ch : apps) if (ch == L'/') ch = L'\\';
        apps += L"steamapps\\";
        std::wstring key = LowerW(apps);
        if (seenSteam.count(key) || !IsDir(apps)) continue;
        seenSteam.insert(key);
        libs_.push_back({ key + L"common\\", "Steam", "steam", "" });
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((apps + L"appmanifest_*.acf").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (!util::ReadFile(apps + fd.cFileName, text)) continue;
            auto names = VdfValues(text, "name"), dirs = VdfValues(text, "installdir");
            if (!names.empty() && !dirs.empty()) steamNames_[key + L"common\\" + LowerW(util::Widen(dirs[0]))] = names[0];
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    // Epic: one manifest per installed game
    WIN32_FIND_DATAW fd;
    std::wstring epic = L"C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests\\";
    HANDLE h = FindFirstFileW((epic + L"*.item").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!util::ReadFile(epic + fd.cFileName, text)) continue;
            Json j = Json::Parse(text);
            std::string loc = j["InstallLocation"].AsString();
            if (!loc.empty()) libs_.push_back({ LowerW(WithSlash(util::Widen(loc))), "Epic", "epic", j["DisplayName"].AsString() });
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    // GOG and Ubisoft Connect keep their installs in the registry
    EachSubkey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GOG.com\\Games", [](HKEY k, const wchar_t* sub, void* ctx) {
        auto* self = (GameDetector*)ctx;
        std::string path = util::RegString(k, sub, L"path"), name = util::RegString(k, sub, L"gameName");
        if (!path.empty()) self->libs_.push_back({ LowerW(WithSlash(util::Widen(path))), "GOG", "gog", name });
    }, this);
    EachSubkey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Ubisoft\\Launcher\\Installs", [](HKEY k, const wchar_t* sub, void* ctx) {
        auto* self = (GameDetector*)ctx;
        std::string dir = util::RegString(k, sub, L"InstallDir");
        for (auto& ch : dir) if (ch == '/') ch = '\\';
        while (!dir.empty() && dir.back() == '\\') dir.pop_back();
        std::string folder = dir.substr(dir.find_last_of('\\') + 1);
        if (!dir.empty()) self->libs_.push_back({ LowerW(WithSlash(util::Widen(dir))), "Ubisoft Connect", "ubisoft", folder });
    }, this);

    // Folders the other stores install into
    wchar_t drives[256];
    DWORD n = GetLogicalDriveStringsW(255, drives);
    for (wchar_t* d = drives; n && *d; d += wcslen(d) + 1) {
        if (GetDriveTypeW(d) != DRIVE_FIXED) continue;
        std::wstring root = LowerW(d);
        if (IsDir(root + L"xboxgames")) libs_.push_back({ root + L"xboxgames\\", "Xbox", "xbox", "" });
        if (IsDir(root + L"riot games")) libs_.push_back({ root + L"riot games\\", "Riot", "riot", "" });
    }
    for (auto* p : { L"C:\\Program Files\\EA Games\\", L"C:\\Program Files (x86)\\Origin Games\\" })
        if (IsDir(p)) libs_.push_back({ LowerW(p), "EA", "ea", "" });
    std::wstring testLib = util::EnvVar(L"OPTM_TEST_LIBRARY");   // testing: treat a folder as a game library
    if (!testLib.empty()) libs_.push_back({ LowerW(WithSlash(testLib)), "Test library", "", "" });

    // Windows' own game list (what Game Bar recognizes as a game)
    EachSubkey(HKEY_CURRENT_USER, L"System\\GameConfigStore\\Children", [](HKEY k, const wchar_t* sub, void* ctx) {
        auto* self = (GameDetector*)ctx;
        std::string exe = util::RegString(k, sub, L"MatchedExeFullPath");
        if (!exe.empty()) self->windowsGames_.insert(LowerW(util::Widen(exe)));
    }, this);

    // most specific root first (a per-game Epic/GOG folder beats a whole drive folder)
    std::stable_sort(libs_.begin(), libs_.end(), [](const Library& a, const Library& b) { return a.root.size() > b.root.size(); });
}

const GameDetector::Library* GameDetector::LibraryOf(const std::wstring& lowerPath) const {
    for (auto& l : libs_) if (lowerPath.compare(0, l.root.size(), l.root) == 0) return &l;
    return nullptr;
}

std::string GameDetector::SteamName(const std::wstring& lowerPath) const {
    for (auto& [dir, name] : steamNames_)
        if (lowerPath.compare(0, dir.size() + 1, dir + L"\\") == 0) return name;
    return "";
}

// ------------------------------------------------------------ deciding
bool GameDetector::LooksLikeHelper(const std::string& exe) {
    static const std::regex re(
        "crash|report|setup|install|unins|update|patch|redist|vc_?redist|dxsetup|directx|dotnet|prereq|bootstrap|launcher|"
        "helper|service|overlay|cef|webview|easyanticheat|battleye|anticheat|_be$|_eac$|unitycrashhandler|crs-|"
        "config|settings|editor|server|uploader|diagnostic|feedback|register|activation|browser|"
        // game engine / modding tools that ship inside game folders (Source, Unreal, Unity)
        "^hammer|hlmv|faceposer|studiomdl|^vbsp|^vrad|^vvis|shadercompile|^vpk$|modkit|sdk|toolkit|^tools?$|^tool_|devkit|creationkit|workshop",
        std::regex::icase);
    return std::regex_search(exe, re);
}

std::string GameDetector::GameNameFromVersionInfo(const std::wstring& path) {
    DWORD dummy = 0, size = GetFileVersionInfoSizeW(path.c_str(), &dummy);
    if (!size) return "";
    std::vector<BYTE> buf(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, buf.data())) return "";
    struct { WORD lang, cp; }* tr = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\VarFileInfo\\Translation", (void**)&tr, &len) || len < 4) return "";
    for (const wchar_t* field : { L"ProductName", L"FileDescription" }) {
        wchar_t q[128];
        swprintf(q, 128, L"\\StringFileInfo\\%04x%04x\\%s", tr->lang, tr->cp, field);
        wchar_t* val = nullptr;
        if (VerQueryValueW(buf.data(), q, (void**)&val, &len) && val && len > 1) {
            std::string s = util::Trim(util::Narrow(val));
            std::string l = util::Lower(s);
            // engine / publisher names aren't game names
            if (s.size() < 2 || l.find("unreal") != std::string::npos || l.find("unity") != std::string::npos ||
                l.find("bootstrap") != std::string::npos || l == "game" || l.find("epic games") != std::string::npos)
                continue;
            return s;
        }
    }
    return "";
}

bool GameDetector::FindAntiCheat(const std::wstring& gameRoot, const std::wstring& exeDir, std::string& which) const {
    static const std::pair<const wchar_t*, const char*> marks[] = {
        { L"EasyAntiCheat", "EasyAntiCheat" }, { L"EasyAntiCheat_EOS", "EasyAntiCheat" }, { L"BattlEye", "BattlEye" },
        { L"EAAntiCheat", "EA Javelin" }, { L"start_protected_game.exe", "EasyAntiCheat" }, { L"EAAntiCheat.GameServiceLauncher.exe", "EA Javelin" },
        { L"mhypbase.dll", "HoYoverse" }, { L"vgk.sys", "Vanguard" },
    };
    std::vector<std::wstring> dirs = { exeDir, Parent(exeDir), Parent(Parent(exeDir)) };
    if (!gameRoot.empty()) dirs.push_back(gameRoot);
    for (auto& d : dirs) {
        if (d.empty()) continue;
        for (auto& [file, name] : marks) if (Exists(d + L"\\" + file)) { which = name; return true; }
        // BattlEye / EAC wrapper exes next to the game ("Game_BE.exe", "Game_EAC.exe")
        WIN32_FIND_DATAW fd;
        for (auto* pat : { L"\\*_BE.exe", L"\\*_EAC.exe" }) {
            HANDLE h = FindFirstFileW((d + pat).c_str(), &fd);
            if (h != INVALID_HANDLE_VALUE) { FindClose(h); which = wcsstr(pat, L"_BE") ? "BattlEye" : "EasyAntiCheat"; return true; }
        }
    }
    return false;
}

bool GameDetector::Evaluate(const std::wstring& path, DWORD pid, DetectedGame& out) {
    std::wstring lower = LowerW(path);
    if (!LibraryOf(lower) && !windowsGames_.count(lower)) return false;
    // needs a real game window (launchers and tools rarely open a full-size one)
    WinCtx c = { pid, false };
    EnumWindows(BigWindow, (LPARAM)&c);
    if (!c.big) return false;
    return Describe(path, out);
}

bool GameDetector::Describe(const std::wstring& path, DetectedGame& out) const {
    std::wstring lower = LowerW(path);
    const Library* lib = LibraryOf(lower);
    if (!lib && !windowsGames_.count(lower)) return false;
    std::wstring file = FileName(path);
    out.path = path;
    out.exe = util::Narrow(file.size() > 4 ? file.substr(0, file.size() - 4) : file);
    out.source = lib ? lib->source : "Windows game list";
    out.keep = lib ? lib->keep : "";

    // name: store data first, then the exe's own version info, then the folder
    std::wstring gameRoot;
    if (lib && lib->source == "Steam") {
        out.name = SteamName(lower);
        size_t end = lower.find(L'\\', lib->root.size());
        if (end != std::wstring::npos) gameRoot = path.substr(0, end);
    } else if (lib) {
        out.name = lib->name;
        size_t end = lower.find(L'\\', lib->root.size());
        gameRoot = lib->name.empty() && end != std::wstring::npos ? path.substr(0, end) : path.substr(0, lib->root.size() - 1);
    }
    if (out.name.empty()) out.name = GameNameFromVersionInfo(path);
    if (out.name.empty() && !gameRoot.empty()) out.name = util::Narrow(FileName(gameRoot));
    if (out.name.empty()) out.name = out.exe;

    out.antiCheat = FindAntiCheat(gameRoot, Parent(path), out.antiCheatName);
    return true;
}

std::vector<DetectedGame> GameDetector::Scan(const ProcessList& procs, const AppData& data) {
    std::vector<DetectedGame> found;
    if (GetTickCount64() - loadedAt_ > 10 * 60 * 1000) Load();   // pick up newly installed games

    std::set<std::string> known;
    for (auto& p : data.profiles) for (auto& e : p.exes) known.insert(util::Lower(e));
    for (auto& e : data.ignoredExes) known.insert(util::Lower(e));
    for (auto& e : data.settings.background) known.insert(util::Lower(e));

    // anti-cheat services running right now (a just-started game's protection shows up here too)
    std::string acRunning;
    for (auto* n : kAntiCheatProcesses) if (procs.Has(n)) { acRunning = n; break; }

    DWORD self = GetCurrentProcessId();
    uint64_t now = GetTickCount64();
    std::set<ProcKey> alive;
    for (DWORD pid : procs.All()) {
        if (pid <= 4 || pid == self) continue;
        std::string name = procs.NameOf(pid);
        std::string lower = util::Lower(name);
        if (known.count(lower)) continue;
        ProcKey k = { pid, proc::CreationTime(pid) };
        if (!k.created) continue;
        alive.insert(k);
        if (decided_.count(k)) continue;
        if (LooksLikeHelper(lower)) { decided_.insert(k); continue; }
        std::wstring path = proc::ImagePath(pid);
        std::wstring lp = LowerW(path);
        if (path.empty() || lp.find(L"\\windows\\") != std::wstring::npos) { decided_.insert(k); continue; }
        if (!LibraryOf(lp) && !windowsGames_.count(lp)) { decided_.insert(k); continue; }
        DetectedGame g;
        if (Evaluate(path, pid, g)) {
            if (!g.antiCheat && !acRunning.empty()) { g.antiCheat = true; g.antiCheatName = acRunning; }
            found.push_back(g);
            known.insert(lower);
            decided_.insert(k);
            pending_.erase(k);
        } else {
            // in a library but no game window yet - keep looking for 90 seconds (loading screens)
            auto it = pending_.find(k);
            if (it == pending_.end()) pending_[k] = now;
            else if (now - it->second > 90000) { decided_.insert(k); pending_.erase(it); }
        }
    }
    // forget processes that have exited
    for (auto it = decided_.begin(); it != decided_.end();) it = alive.count(*it) ? std::next(it) : decided_.erase(it);
    for (auto it = pending_.begin(); it != pending_.end();) it = alive.count(it->first) ? std::next(it) : pending_.erase(it);
    return found;
}
