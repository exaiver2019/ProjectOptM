#include "data.h"
#include "json.h"
#include "util.h"
#include "version.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace {

// Written to %APPDATA%\ProjectOptM\profiles.ini when there isn't one yet (same as 1.1's defaults)
const char* kDefaultIni =
R"INI(; ================================================================
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
;   cores         Best   = picked from your CPU         Best
;                 Prefer = like Best, but steered
;                          instead of locked (good for
;                          games using many threads)
;                 Other  = non-V-Cache CCD (X3D only)
;                 All    = no pinning
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
;   tweaks        Tweaks preset for this game (Safe,    Tweaks page
;                 Balanced, Aggressive or one of yours)  preset
;                 or custom: id, id, ... - easiest to
;                 set with the gear on the Games page
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
)INI";

bool Yes(const std::string& v, bool def) {
    if (v.empty()) return def;
    std::string l = util::Lower(v);
    return l == "yes" || l == "true" || l == "on" || l == "1";
}

std::string NoSpaces(std::string s) {
    s = util::Lower(s);
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    return s;
}

// profiles.ini: [Game] sections with key = value lines; list keys may repeat
void ParseProfiles(const std::string& text, AppData& d) {
    std::vector<std::pair<std::string, std::map<std::string, std::string>>> sections;
    const char* listKeys[] = { "exe", "boost", "background", "close", "pause_services", "keep" };
    std::istringstream in(text);
    std::string raw;
    while (std::getline(in, raw)) {
        std::string line = util::Trim(raw);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        // strip trailing comments ("value   ; note")
        for (size_t i = 1; i < line.size(); i++)
            if ((line[i] == ';' || line[i] == '#') && (line[i - 1] == ' ' || line[i - 1] == '\t')) { line = util::Trim(line.substr(0, i)); break; }
        if (line.front() == '[' && line.back() == ']') {
            std::string name = util::Trim(line.substr(1, line.size() - 2));
            auto it = std::find_if(sections.begin(), sections.end(), [&](auto& s) { return s.first == name; });
            if (it == sections.end()) sections.push_back({ name, {} });
            else { auto keep = *it; sections.erase(it); sections.push_back(keep); }   // repeated section: keep adding to it
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos || sections.empty()) continue;
        std::string k = util::Lower(util::Trim(line.substr(0, eq)));
        std::string v = util::Trim(line.substr(eq + 1));
        auto& sec = sections.back().second;
        bool isList = false;
        for (auto* lk : listKeys) if (k == lk) isList = true;
        if (isList && sec.count(k)) sec[k] += "," + v; else sec[k] = v;
    }
    for (auto& [name, sec] : sections) {
        if (name == "Settings") {
            IniSettings& s = d.settings;
            if (!sec["check_every"].empty()) {
                int n = atoi(sec["check_every"].c_str());
                if (n >= 1 && n <= 30) s.poll = n;
                else d.warnings.push_back("check_every '" + sec["check_every"] + "' isn't 1-30, using 3");
            }
            s.background      = util::NameList(sec["background"]);
            s.pauseUpdates    = Yes(sec["pause_updates"], true);
            s.forceGpu        = Yes(sec["force_dedicated_gpu"], true);
            s.cleanupOnLaunch = Yes(sec["cleanup_on_launch"], true);
            s.close           = util::NameList(sec["close"]);
            if (sec.count("pause_services")) {
                s.pauseServices.clear();
                for (auto& x : util::NameList(sec["pause_services"])) {
                    std::string l = util::Lower(x);
                    if (l != "no" && l != "off" && l != "none") s.pauseServices.push_back(x);
                }
            }
            s.panicHotkey  = Yes(sec["panic_hotkey"], true);
            s.reopenClosed = Yes(sec["reopen_closed"], true);
            s.vendorApps   = Yes(sec["vendor_apps"], true);
            std::string bc = util::Lower(sec["background_cores"]);
            if (bc == "auto" || bc == "off") s.backgroundCores = bc;
            else if (!bc.empty()) d.warnings.push_back("background_cores '" + sec["background_cores"] + "' unknown, using auto");
            std::string pp = util::Lower(sec["power_plan"]);
            if (pp == "auto" || pp == "high" || pp == "off") s.powerPlan = pp;
            else if (!pp.empty()) d.warnings.push_back("power_plan '" + sec["power_plan"] + "' unknown, using auto");
            continue;
        }
        GameProfile p;
        p.name = name;
        p.exes = util::NameList(sec["exe"]);
        if (p.exes.empty()) { d.warnings.push_back("[" + name + "] has no exe - skipped"); continue; }
        p.antiCheat = Yes(sec["anticheat"], false);
        std::string pr = NoSpaces(sec["priority"]);
        if (pr == "normal") p.priority = "Normal";
        else if (pr == "high") p.priority = "High";
        else {
            p.priority = "AboveNormal";
            if (!pr.empty() && pr != "abovenormal") d.warnings.push_back("[" + name + "] priority '" + sec["priority"] + "' unknown, using AboveNormal");
        }
        std::string c = util::Lower(sec["cores"]);
        if (c == "other" || c == "frequency") p.cores = "Other";
        else if (c == "all") p.cores = "All";
        else if (c == "prefer" || c == "soft") { p.cores = "Best"; p.softPin = true; }
        else {
            p.cores = "Best";
            if (!c.empty() && c != "best" && c != "vcache") d.warnings.push_back("[" + name + "] cores '" + sec["cores"] + "' unknown, using Best");
        }
        const std::string& rc = sec["ramcleanup"];
        if (!rc.empty()) {
            char* end = nullptr;
            long n = strtol(rc.c_str(), &end, 10);
            if (end && *end == 0 && n >= 0) p.ramCleanupMins = (int)n;
            else d.warnings.push_back("[" + name + "] ramcleanup '" + rc + "' isn't a number, turned off");
        }
        p.boost = util::NameList(sec["boost"]);
        p.keep = util::NameList(sec["keep"]);
        p.close = util::NameList(sec["close"]);
        std::string lp = NoSpaces(sec["launch_priority"]);
        if (lp == "normal") p.launchPriority = "Normal";
        else if (lp == "abovenormal") p.launchPriority = "AboveNormal";
        else if (lp == "high") p.launchPriority = "High";
        else if (!lp.empty() && lp != "off" && lp != "no" && lp != "none")
            d.warnings.push_back("[" + name + "] launch_priority '" + sec["launch_priority"] + "' unknown, turned off");
        p.ecoQosOff = Yes(sec["ecoqos_off"], true);
        // tweaks = Balanced (a preset) | custom: id, id, ... (this game's own set) | empty = the Tweaks page preset
        std::string tw = util::Trim(sec["tweaks"]);
        if (util::Lower(tw) == "custom" || util::Lower(tw).rfind("custom:", 0) == 0) {
            p.tweaks = "Custom";
            size_t colon = tw.find(':');
            if (colon != std::string::npos)
                for (auto& id : util::Split(tw.substr(colon + 1), ',')) if (!util::Trim(id).empty()) p.tweakIds.push_back(util::Lower(util::Trim(id)));
        } else if (!tw.empty() && util::Lower(tw) != "default") p.tweaks = tw;
        d.profiles.push_back(p);
    }
}

std::vector<std::string> CsvFields(const std::string& line) {
    std::vector<std::string> f;
    std::string cur;
    bool q = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (q) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
            else if (c == '"') q = false;
            else cur += c;
        } else if (c == '"') q = true;
        else if (c == ',') { f.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    f.push_back(cur);
    return f;
}

std::string CsvQuote(const std::string& s) {
    std::string o = "\"";
    for (char c : s) { if (c == '"') o += '"'; o += c; }
    return o + "\"";
}

std::string Num1(double v) {   // "12.5" / "" for zero - invariant, like the 1.1 app writes it
    if (v <= 0) return "";
    char b[32]; snprintf(b, sizeof(b), "%.1f", v);
    std::string s = b;
    if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0) s.resize(s.size() - 2);
    return s;
}

// "Mon Sep  1 14:03:22 2025" -> "2025-09-01 14:03:22"
std::string ParseCtime(const std::string& s) {
    char wd[8] = {}, mon[8] = {};
    int d, h, mi, se, y;
    if (sscanf(s.c_str(), "%7s %7s %d %d:%d:%d %d", wd, mon, &d, &h, &mi, &se, &y) != 7) return {};
    static const char* M[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
    int m = 0;
    for (int i = 0; i < 12; i++) if (_stricmp(mon, M[i]) == 0) m = i + 1;
    if (!m) return {};
    char b[32]; snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, mi, se);
    return b;
}

const char* kCsvHeader = "\"Date\",\"Game\",\"Minutes\",\"AvgFps\",\"Low1\",\"Version\"\r\n";

void AppendCsv(const std::wstring& path, const std::string& rows) {
    std::string existing;
    bool have = util::ReadFile(path, existing) && !existing.empty();
    std::string out;
    if (!have) out = kCsvHeader;
    else if (existing.back() != '\n') out = "\r\n";
    out += rows;
    util::AppendFile(path, out);
}

}  // namespace

std::string PriorityLabel(const std::string& p) { return p.empty() ? "Off" : p == "AboveNormal" ? "Above normal" : p; }

std::wstring AppData::DataDir() const { return util::AppDataDir(); }
std::wstring AppData::ProfilesPath() const { return DataDir() + L"\\profiles.ini"; }
std::wstring AppData::HistoryPath() const { return DataDir() + L"\\history.csv"; }
std::wstring AppData::DetailsPath() const { return DataDir() + L"\\session-details.json"; }

void AppData::Load() {
    LoadProfiles();
    LoadConfig();
    LoadHistory();
    std::string text;
    timeline.clear();
    if (util::ReadFile(DataDir() + L"\\timeline.json", text)) timeline = Json::Parse(text)["Events"].AsStrings();
}

std::string AppData::UpdateChannel() const {
    if (!updateChannel.empty()) return updateChannel;
    return std::string(OPTM_CHANNEL).empty() ? "stable" : "experimental";   // follow the build you run
}

namespace {
const char* kBackupFiles[] = { "profiles.ini", "settings.json", "history.csv", "session-details.json", "timeline.json" };

// settings.json keys that describe this PC's current state, not your choices: never exported, never
// restored (restoring another PC's crash-recovery list would "undo" changes that were never made here)
void KeepMachineState(Json& to, const Json& from) {
    for (const char* k : { "Ifeo", "GpuManaged", "RestorePlan", "PausedSvcs", "OptimizerImport" }) {
        if (from[k].type == Json::Null) to.obj.erase(k); else to.obj[k] = from[k];
    }
    if (to["Tweaks"].type == Json::Object) {
        for (const char* k : { "Backups", "FsoManaged" }) {
            if (from["Tweaks"][k].type == Json::Null) to.obj["Tweaks"].obj.erase(k); else to.obj["Tweaks"].obj[k] = from["Tweaks"][k];
        }
    }
    if (to["Experimental"].type == Json::Object) {
        if (from["Experimental"]["GpuDriverSeen"].type == Json::Null) to.obj["Experimental"].obj.erase("GpuDriverSeen");
        else to.obj["Experimental"].obj["GpuDriverSeen"] = from["Experimental"]["GpuDriverSeen"];
    }
}
}  // namespace

bool AppData::ExportBackup(const std::wstring& file, std::string& error) const {
    Json files = Json::Obj();
    for (const char* name : kBackupFiles) {
        std::string text;
        if (!util::ReadFile(DataDir() + L"\\" + util::Widen(name), text)) continue;
        if (std::string(name) == "settings.json") {   // your choices only
            Json s = Json::Parse(text);
            KeepMachineState(s, Json::Obj());
            text = s.Dump() + "\r\n";
        }
        files.obj[name] = Json::Str(text);
    }
    if (files.obj.empty()) { error = "there's nothing to back up yet"; return false; }
    Json j = Json::Obj();
    j.obj["ProjectOptMBackup"] = Json::Num(1);
    j.obj["Created"] = Json::Str(util::NowStamp("%Y-%m-%d %H:%M"));
    j.obj["Version"] = Json::Str(OPTM_VERSION_LABEL);
    j.obj["Files"] = files;
    if (!util::WriteFile(file, j.Dump() + "\r\n")) { error = "couldn't write the file"; return false; }
    return true;
}

bool AppData::ImportBackup(const std::wstring& file, std::string& error) {
    std::string text;
    if (!util::ReadFile(file, text)) { error = "couldn't read the file"; return false; }
    if (text.size() > 64u * 1024 * 1024) { error = "that file is far too big to be a backup"; return false; }
    Json j = Json::Parse(text);
    if (j["ProjectOptMBackup"].type != Json::Number || j["Files"].type != Json::Object) { error = "that isn't a Project OptM backup"; return false; }
    const Json& files = j["Files"];
    if (files["profiles.ini"].type != Json::String) { error = "the backup has no game profiles in it"; return false; }
    // keep what's here now, in case the restore isn't what you wanted
    std::wstring keep = DataDir() + L"\\backup-before-restore-" + util::Widen(util::NowStamp("%Y%m%d-%H%M%S"));
    CreateDirectoryW(keep.c_str(), nullptr);
    for (const char* name : kBackupFiles) {
        std::wstring n = util::Widen(name);
        CopyFileW((DataDir() + L"\\" + n).c_str(), (keep + L"\\" + n).c_str(), FALSE);
    }
    for (const char* name : kBackupFiles) {
        const Json& f = files[name];
        if (f.type != Json::String) continue;   // not in the backup: yours stays
        std::string out = f.str;
        if (std::string(name) == "settings.json") {
            std::string cur;
            Json now = util::ReadFile(DataDir() + L"\\settings.json", cur) ? Json::Parse(cur) : Json::Obj();
            Json s = Json::Parse(out);
            if (s.type != Json::Object) continue;
            KeepMachineState(s, now);   // this PC's recovery state stays as it is
            out = "\xEF\xBB\xBF" + s.Dump() + "\r\n";
        }
        if (!util::WriteFile(DataDir() + L"\\" + util::Widen(name), out)) { error = std::string("couldn't write ") + name + " (your old files are in " + util::Narrow(keep) + ")"; return false; }
    }
    Load();
    return true;
}

void AppData::AddTimeline(const std::string& kind, const std::string& text) {
    timeline.push_back(util::NowStamp("%Y-%m-%d %H:%M:%S") + "|" + kind + "|" + text);
    if (timeline.size() > 400) timeline.erase(timeline.begin(), timeline.end() - 400);
    Json j = Json::Obj();
    j.obj["Events"] = Json::StrList(timeline);
    CreateDirectoryW(DataDir().c_str(), nullptr);
    util::WriteFile(DataDir() + L"\\timeline.json", j.Dump() + "\r\n");
}

namespace {
// session-details.json: {"Sessions": {"<date>|<game>": {...}}} - the extra numbers 2.1.1 records
Json DetailsOf(const Session& s) {
    Json d = Json::Obj();
    auto num = [&](const char* k, double v) { if (v >= 0) d.obj[k] = Json::Num(std::round(v * 10) / 10); };
    num("Low01", s.low01 > 0 ? s.low01 : -1);
    if (s.stutters >= 0) d.obj["Stutters"] = Json::Num(s.stutters);
    num("FpsSeconds", s.fpsSeconds > 0 ? s.fpsSeconds : -1);
    auto str = [&](const char* k, const std::string& v) { if (!v.empty()) d.obj[k] = Json::Str(v); };
    str("Exit", s.exit); str("Preset", s.preset); str("Cores", s.cores); str("Variant", s.variant); str("Cause", s.cause);
    num("GpuTempAvg", s.gpuTempAvg); num("GpuTempMax", s.gpuTempMax); num("CpuAvg", s.cpuAvg); num("PingAvg", s.pingAvg);
    if (!s.tweaks.empty()) d.obj["Tweaks"] = Json::StrList(s.tweaks);
    if (s.hz > 0) d.obj["Hz"] = Json::Num(s.hz);
    num("Fps5", s.fps5 > 0 ? s.fps5 : -1);
    if (s.hotSeconds > 0) d.obj["HotSeconds"] = Json::Num(s.hotSeconds);
    num("HeatDrop", s.heatDrop > 0 ? s.heatDrop : -1);
    if (s.otherVideoSeconds > 0) d.obj["OtherVideoSeconds"] = Json::Num(s.otherVideoSeconds);
    if (s.loadStutters > 0) d.obj["LoadStutters"] = Json::Num(s.loadStutters);
    if (s.loadSeconds > 0) d.obj["LoadSeconds"] = Json::Num(s.loadSeconds);
    return d;
}
void ApplyDetails(Session& s, const Json& d) {
    auto num = [&](const char* k, double def) { return d[k].type == Json::Number ? d[k].num : def; };
    s.low01 = num("Low01", 0);
    s.stutters = (int)num("Stutters", -1);
    s.fpsSeconds = num("FpsSeconds", 0);
    s.exit = d["Exit"].AsString(); s.preset = d["Preset"].AsString(); s.cores = d["Cores"].AsString();
    s.variant = d["Variant"].AsString(); s.cause = d["Cause"].AsString();
    s.gpuTempAvg = num("GpuTempAvg", -1); s.gpuTempMax = num("GpuTempMax", -1); s.cpuAvg = num("CpuAvg", -1); s.pingAvg = num("PingAvg", -1);
    s.tweaks = d["Tweaks"].AsStrings();
    s.hz = (int)num("Hz", 0);
    s.fps5 = num("Fps5", 0);
    s.hotSeconds = (int)num("HotSeconds", 0);
    s.heatDrop = num("HeatDrop", 0);
    s.otherVideoSeconds = (int)num("OtherVideoSeconds", 0);
    s.loadStutters = (int)num("LoadStutters", 0);
    s.loadSeconds = (int)num("LoadSeconds", 0);
}
}  // namespace

bool AppData::LoadProfiles() {
    std::string text;
    if (!util::ReadFile(ProfilesPath(), text)) {
        CreateDirectoryW(DataDir().c_str(), nullptr);
        std::string def = kDefaultIni;
        std::string crlf;
        for (char c : def) { if (c == '\n') crlf += '\r'; crlf += c; }
        util::WriteFile(ProfilesPath(), crlf);
        profilesCreated = true;
        if (!util::ReadFile(ProfilesPath(), text)) text = def;
    }
    profiles.clear(); warnings.clear();
    settings = IniSettings();
    ParseProfiles(text, *this);
    profilesStamp = util::FileTime(ProfilesPath());
    return true;
}

bool AppData::ProfilesChanged() const { return util::FileTime(ProfilesPath()) != profilesStamp; }

void AppData::LoadConfig() {
    launchPaths.clear();
    std::string text;
    if (!util::ReadFile(DataDir() + L"\\settings.json", text)) {
        // settings from the old "GameOptimizer" name
        std::wstring old = util::AppDataRoot() + L"\\GameOptimizer\\settings.json";
        if (!util::ReadFile(old, text)) return;
    }
    Json j = Json::Parse(text);
    for (auto& [k, v] : j["Paths"].obj) launchPaths[k] = v.AsString();
    autoOptimize = j["Auto"].AsBool(true);
    fpsOn = j["FpsOn"].AsBool(true);
    autoUpdate = j["AutoUpdate"].AsBool(true);
    uiScale = j["UiScale"].type == Json::Number ? j["UiScale"].num : 0;
    if (uiScale != 0 && (uiScale < 0.75 || uiScale > 3)) uiScale = 0;
    const Json& tw = j["Tweaks"];
    tweakPreset = tw["Preset"].AsString("Safe");
    tweakPresets.clear();
    for (auto& [name, ids] : tw["Custom"].obj) tweakPresets[name] = ids.AsStrings();
    tweakBackups = tw["Backups"].AsStrings();
    fsoManaged = tw["FsoManaged"].AsStrings();
    autoDetect = j["AutoDetect"].AsBool(true);
    animations = j["Animations"].AsBool(true);
    intro = j["Intro"].AsBool(true);
    greetName = j["Name"].AsString();
    tourDone = j["TourDone"].AsBool(false);
    ignoredExes = j["IgnoredExes"].AsStrings();
    autoAdded = j["AutoAdded"].AsStrings();
    if (tweakPreset != "Safe" && tweakPreset != "Balanced" && tweakPreset != "Aggressive" && !tweakPresets.count(tweakPreset)) tweakPreset = "Safe";
    ifeoManaged = j["Ifeo"].AsStrings();
    gpuManaged = j["GpuManaged"].AsStrings();
    revertOnExit = j["RevertOnExit"].AsBool(true);
    const Json& ov = j["Overlay"];
    overlayOn = ov["On"].AsBool(false);
    if (ov["X"].type == Json::Number && ov["Y"].type == Json::Number) {
        overlayX = std::clamp(ov["X"].num, 0.0, 1.0);
        overlayY = std::clamp(ov["Y"].num, 0.0, 1.0);
    } else {   // 2.1 test builds kept a corner (0 top-left ... 3 bottom-right)
        int corner = ov["Corner"].type == Json::Number ? std::clamp((int)ov["Corner"].num, 0, 3) : 0;
        overlayX = (corner == 1 || corner == 3) ? 1 : 0;
        overlayY = corner >= 2 ? 1 : 0;
    }
    if (ov["Opacity"].type == Json::Number) overlayOpacity = std::clamp((int)ov["Opacity"].num, 0, 100);
    if (ov["Size"].type == Json::Number) overlaySize = std::clamp((int)ov["Size"].num, 70, 200);
    if (ov["Rate"].type == Json::Number) overlayRate = std::clamp((int)ov["Rate"].num, 1, 30);
    if (ov["Items"].type == Json::Array) overlayItems = ov["Items"].AsStrings();
    else if (!ov["Graph"].AsBool(true)) overlayItems = { "low", "frametime" };
    overlayAntiCheat = ov["AntiCheat"].AsBool(false);
    restorePlan = j["RestorePlan"].AsString();
    pausedServices = j["PausedSvcs"].AsStrings();
    optimizerImport = j["OptimizerImport"].AsString();
    const Json& ex = j["Experimental"];
    tests.clear();
    for (auto& [g, v] : ex["Tests"].obj) if (v.type == Json::String) tests[g] = v.str;
    latencyOn = ex["LatencyTrace"].AsBool(false);
    pingOn = ex["Ping"].AsBool(false);
    askGames = ex["AskGames"].AsBool(true);
    gpuDriverSeen = ex["GpuDriverSeen"].AsString();
    summaryOn = ex["Summary"].AsBool(true);
    updateChannel = j["UpdateChannel"].AsString();
    if (updateChannel != "stable" && updateChannel != "experimental") updateChannel.clear();
    const Json& t = j["Theme"];
    theme.accent     = t["Accent"].AsString(theme.accent);
    theme.background = t["Bg"].AsString(theme.background);
    theme.corners    = t["Corners"].AsString(theme.corners);
}

// Same keys the 1.1 app writes, so switching back and forth keeps everything
void AppData::SaveConfig() const {
    std::wstring path = DataDir() + L"\\settings.json";
    std::string text;
    Json j = util::ReadFile(path, text) ? Json::Parse(text) : Json::Obj();
    if (j.type != Json::Object) j = Json::Obj();
    Json p = Json::Obj();
    for (auto& [k, v] : launchPaths) p.obj[k] = Json::Str(v);
    j.obj["Paths"] = p;
    j.obj["Auto"] = Json::Boolean(autoOptimize);
    j.obj["Ifeo"] = Json::StrList(ifeoManaged);
    j.obj["GpuManaged"] = Json::StrList(gpuManaged);
    j.obj["RevertOnExit"] = Json::Boolean(revertOnExit);
    Json ov = Json::Obj();
    ov.obj["On"] = Json::Boolean(overlayOn);
    ov.obj["X"] = Json::Num(overlayX);
    ov.obj["Y"] = Json::Num(overlayY);
    ov.obj["Opacity"] = Json::Num(overlayOpacity);
    ov.obj["Size"] = Json::Num(overlaySize);
    ov.obj["Rate"] = Json::Num(overlayRate);
    ov.obj["Items"] = Json::StrList(overlayItems);
    ov.obj["AntiCheat"] = Json::Boolean(overlayAntiCheat);
    j.obj["Overlay"] = ov;
    j.obj["RestorePlan"] = restorePlan.empty() ? Json() : Json::Str(restorePlan);
    Json t = Json::Obj();
    t.obj["Accent"] = Json::Str(theme.accent);
    t.obj["Bg"] = Json::Str(theme.background);
    t.obj["Corners"] = Json::Str(theme.corners);
    j.obj["Theme"] = t;
    j.obj["FpsOn"] = Json::Boolean(fpsOn);
    j.obj["AutoUpdate"] = Json::Boolean(autoUpdate);
    j.obj["UiScale"] = Json::Num(uiScale);
    Json tw = Json::Obj();
    tw.obj["Preset"] = Json::Str(tweakPreset);
    Json custom = Json::Obj();
    for (auto& [name, ids] : tweakPresets) custom.obj[name] = Json::StrList(ids);
    tw.obj["Custom"] = custom;
    tw.obj["Backups"] = Json::StrList(tweakBackups);
    tw.obj["FsoManaged"] = Json::StrList(fsoManaged);
    j.obj["Tweaks"] = tw;
    j.obj["AutoDetect"] = Json::Boolean(autoDetect);
    j.obj["Animations"] = Json::Boolean(animations);
    j.obj["Intro"] = Json::Boolean(intro);
    j.obj["Name"] = greetName.empty() ? Json() : Json::Str(greetName);
    j.obj["TourDone"] = Json::Boolean(tourDone);
    j.obj["IgnoredExes"] = Json::StrList(ignoredExes);
    j.obj["AutoAdded"] = Json::StrList(autoAdded);
    j.obj["PausedSvcs"] = Json::StrList(pausedServices);
    j.obj["OptimizerImport"] = optimizerImport.empty() ? Json() : Json::Str(optimizerImport);
    Json ex = Json::Obj();
    Json tj = Json::Obj();
    for (auto& [g, v] : tests) tj.obj[g] = Json::Str(v);
    ex.obj["Tests"] = tj;
    ex.obj["LatencyTrace"] = Json::Boolean(latencyOn);
    ex.obj["Ping"] = Json::Boolean(pingOn);
    ex.obj["AskGames"] = Json::Boolean(askGames);
    ex.obj["GpuDriverSeen"] = gpuDriverSeen.empty() ? Json() : Json::Str(gpuDriverSeen);
    ex.obj["Summary"] = Json::Boolean(summaryOn);
    j.obj["Experimental"] = ex;
    j.obj["UpdateChannel"] = updateChannel.empty() ? Json() : Json::Str(updateChannel);
    CreateDirectoryW(DataDir().c_str(), nullptr);
    util::WriteFile(path, "\xEF\xBB\xBF" + j.Dump() + "\r\n");   // BOM so Windows PowerShell 5.1 reads it as UTF-8
}

void AppData::LoadHistory() {
    history.clear(); playtime.clear();
    std::string text;
    if (!util::ReadFile(HistoryPath(), text)) return;
    std::istringstream in(text);
    std::string line;
    std::vector<std::string> header;
    while (std::getline(in, line)) {
        if (util::Trim(line).empty()) continue;
        auto f = CsvFields(line);
        if (header.empty()) { header = f; continue; }
        Session s;
        for (size_t i = 0; i < f.size() && i < header.size(); i++) {
            const std::string& h = header[i];
            if (h == "Date") s.date = f[i];
            else if (h == "Game") s.game = f[i];
            else if (h == "Minutes") s.minutes = atof(f[i].c_str());
            else if (h == "AvgFps") s.avgFps = atof(f[i].c_str());
            else if (h == "Low1") s.low1 = atof(f[i].c_str());
            else if (h == "Version") s.version = f[i];
        }
        if (s.game.empty()) continue;
        history.push_back(s);
        playtime[s.game] += s.minutes;
    }
    if (util::ReadFile(DetailsPath(), text)) {
        Json j = Json::Parse(text);
        const Json& all = j["Sessions"];
        for (auto& s : history) {
            auto it = all.obj.find(s.date + "|" + s.game);
            if (it != all.obj.end()) ApplyDetails(s, it->second);
        }
    }
}

void AppData::AddSession(const Session& s, const std::string& version) {
    char mins[32]; snprintf(mins, sizeof(mins), "%.1f", s.minutes);
    std::string row = CsvQuote(s.date) + "," + CsvQuote(s.game) + "," + CsvQuote(mins) + "," +
                      CsvQuote(Num1(s.avgFps)) + "," + CsvQuote(Num1(s.low1)) + "," + CsvQuote(version) + "\r\n";
    CreateDirectoryW(DataDir().c_str(), nullptr);
    AppendCsv(HistoryPath(), row);
    Session copy = s; copy.version = version;
    history.push_back(copy);
    playtime[s.game] += s.minutes;
    // the details, keyed by date + game (two sessions of a game in one minute: the later one wins)
    Json d = DetailsOf(s);
    if (d.obj.empty()) return;
    std::string text;
    Json j = util::ReadFile(DetailsPath(), text) ? Json::Parse(text) : Json::Obj();
    if (j.type != Json::Object) j = Json::Obj();
    if (j["Sessions"].type != Json::Object) j.obj["Sessions"] = Json::Obj();
    j.obj["Sessions"].obj[s.date + "|" + s.game] = d;
    util::WriteFile(DetailsPath(), j.Dump() + "\r\n");
}

std::string AppData::AppendProfile(const std::string& baseName, const std::string& exe, const std::string& source,
                                   const std::string& keep, bool antiCheat, const std::string& antiCheatName) {
    // section names can't hold brackets; keep them unique
    std::string clean;
    for (char c : baseName) if (c != '[' && c != ']' && c != '\r' && c != '\n') clean += c;
    clean = util::Trim(clean);
    if (clean.empty()) clean = exe;
    std::string name = clean;
    auto taken = [&](const std::string& n) {
        if (util::Lower(n) == "settings") return true;
        for (auto& p : profiles) if (util::Lower(p.name) == util::Lower(n)) return true;
        return false;
    };
    for (int i = 2; taken(name); i++) name = clean + " (" + std::to_string(i) + ")";

    std::string how = source == "a share code" ? "added from a share code (" : source == "you said it's a game" ? "added when you said it's a game ("
                    : "added automatically when it started (" + source + ", ";
    std::string block = "\r\n; ---- " + how + util::NowStamp("%Y-%m-%d") + ") - edit or delete freely\r\n[" + name + "]\r\n";
    auto line = [&](const char* k, const std::string& v) { char b[64]; snprintf(b, sizeof(b), "%-17s= ", k); block += b + v + "\r\n"; };
    line("exe", exe);
    if (antiCheat) {
        line("anticheat", "yes          ; " + (antiCheatName.empty() ? std::string("anti-cheat") : antiCheatName) + " found - game process is never touched");
        line("launch_priority", "AboveNormal");
    } else {
        line("priority", "AboveNormal");
        line("cores", "Best");
    }
    if (!keep.empty()) line("keep", keep);

    std::string text;
    util::ReadFile(ProfilesPath(), text);
    if (!text.empty() && text.back() != '\n') block = "\r\n" + block;
    util::AppendFile(ProfilesPath(), block);
    return name;
}

bool AppData::RemoveProfile(const std::string& name) {
    std::string text;
    if (!util::ReadFile(ProfilesPath(), text)) return false;
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string l;
    while (std::getline(in, l)) { if (!l.empty() && l.back() == '\r') l.pop_back(); lines.push_back(l); }
    size_t start = std::string::npos;
    for (size_t i = 0; i < lines.size(); i++)
        if (util::Trim(lines[i]) == "[" + name + "]") { start = i; break; }
    if (start == std::string::npos) return false;
    size_t end = start + 1;
    while (end < lines.size() && util::Trim(lines[end]).rfind("[", 0) != 0) end++;
    // don't take the next block's comment lines with us
    while (end > start + 1 && (util::Trim(lines[end - 1]).empty() || util::Trim(lines[end - 1])[0] == ';')) end--;
    // take our own "added automatically" note and the blank line before it
    if (start > 0 && util::Trim(lines[start - 1]).rfind("; ---- added automatically", 0) == 0) start--;
    if (start > 0 && util::Trim(lines[start - 1]).empty()) start--;
    lines.erase(lines.begin() + start, lines.begin() + end);
    std::string out;
    for (auto& x : lines) out += x + "\r\n";
    return util::WriteFile(ProfilesPath(), out);
}

bool AppData::SaveProfile(const GameProfile& p) {
    std::string text;
    if (!util::ReadFile(ProfilesPath(), text)) return false;
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string l;
    while (std::getline(in, l)) { if (!l.empty() && l.back() == '\r') l.pop_back(); lines.push_back(l); }
    // the last [name] block wins when a section repeats, so edit the last one
    size_t start = std::string::npos;
    for (size_t i = 0; i < lines.size(); i++)
        if (util::Trim(lines[i]) == "[" + p.name + "]") start = i;
    if (start == std::string::npos) return false;
    size_t end = start + 1;
    while (end < lines.size() && util::Trim(lines[end]).rfind("[", 0) != 0) end++;

    // drop the keys we write; remember where they were and any note after a value
    const char* managed[] = { "exe", "anticheat", "priority", "cores", "ramcleanup", "boost", "launch_priority",
                              "close", "keep", "ecoqos_off", "tweaks" };
    std::map<std::string, std::pair<size_t, std::string>> notes;   // key -> column, "; note"
    std::vector<std::string> kept;
    size_t insertAt = std::string::npos, keyW = 17;                 // keyW: this block's "key   =" width
    for (size_t i = start + 1; i < end; i++) {
        const std::string& t = lines[i];
        std::string tt = util::Trim(t);
        size_t eq = t.find('=');
        std::string k = eq == std::string::npos || tt.empty() || tt[0] == ';' || tt[0] == '#' ? "" : util::Lower(util::Trim(t.substr(0, eq)));
        bool ours = false;
        for (auto* m : managed) if (k == m) ours = true;
        if (!ours) { kept.push_back(t); continue; }
        if (insertAt == std::string::npos) { insertAt = kept.size(); keyW = std::max(eq, (size_t)12); }
        for (size_t c = eq + 1; c < t.size(); c++)
            if ((t[c] == ';' || t[c] == '#') && (t[c - 1] == ' ' || t[c - 1] == '\t')) { notes[k] = { c, t.substr(c) }; break; }
    }
    if (insertAt == std::string::npos) {   // no keys yet: right after the header, before trailing notes/blank lines
        insertAt = kept.size();
        while (insertAt > 0 && (util::Trim(kept[insertAt - 1]).empty() || util::Trim(kept[insertAt - 1])[0] == ';')) insertAt--;
    }

    std::vector<std::string> block;
    auto line = [&](const char* k, const std::string& v) {
        char b[64]; snprintf(b, sizeof(b), "%-*s= ", (int)keyW, k);
        std::string s = b + v;
        auto n = notes.find(k);
        if (n != notes.end()) { if (s.size() < n->second.first) s.resize(n->second.first, ' '); else s += "   "; s += n->second.second; }
        block.push_back(s);
    };
    line("exe", util::Join(p.exes, ", "));
    if (p.antiCheat) line("anticheat", "yes");
    if (!p.antiCheat || p.priority != "AboveNormal") line("priority", p.priority);
    if (!p.antiCheat || p.cores != "Best" || p.softPin) line("cores", p.softPin && p.cores == "Best" ? "Prefer" : p.cores);
    if (!p.launchPriority.empty()) line("launch_priority", p.launchPriority);
    if (p.ramCleanupMins > 0) line("ramcleanup", std::to_string(p.ramCleanupMins));
    if (!p.boost.empty()) line("boost", util::Join(p.boost, ", "));
    if (!p.close.empty()) line("close", util::Join(p.close, ", "));
    if (!p.keep.empty()) line("keep", util::Join(p.keep, ", "));
    if (!p.ecoQosOff) line("ecoqos_off", "no");
    if (p.tweaks == "Custom") line("tweaks", "custom: " + util::Join(p.tweakIds, ", "));
    else if (!p.tweaks.empty()) line("tweaks", p.tweaks);
    kept.insert(kept.begin() + insertAt, block.begin(), block.end());

    lines.erase(lines.begin() + start + 1, lines.begin() + end);
    lines.insert(lines.begin() + start + 1, kept.begin(), kept.end());
    std::string out;
    for (auto& x : lines) out += x + "\r\n";
    return util::WriteFile(ProfilesPath(), out);
}

int AppData::ImportOtherHistory() {
    int added = 0;
    std::string text;

    // 1. history-import.csv (one-time import, renamed when done)
    std::wstring importPath = DataDir() + L"\\history-import.csv";
    if (util::ReadFile(importPath, text)) {
        std::istringstream in(text);
        std::string line, rows;
        std::vector<std::string> header;
        while (std::getline(in, line)) {
            if (util::Trim(line).empty()) continue;
            auto f = CsvFields(line);
            if (header.empty()) { header = f; continue; }
            std::map<std::string, std::string> r;
            for (size_t i = 0; i < f.size() && i < header.size(); i++) r[header[i]] = f[i];
            rows += CsvQuote(r["Date"]) + "," + CsvQuote(r["Game"]) + "," + CsvQuote(r["Minutes"]) + "," +
                    CsvQuote(r["AvgFps"]) + "," + CsvQuote(r["Low1"]) + "," + CsvQuote(r["Version"]) + "\r\n";
            added++;
        }
        if (!rows.empty()) AppendCsv(HistoryPath(), rows);
        MoveFileExW(importPath.c_str(), (DataDir() + L"\\history-import.done.csv").c_str(), MOVEFILE_REPLACE_EXISTING);
    }

    // 2. the "Optimizer" app's session log - only sessions newer than the last import
    std::wstring src = util::LocalAppDataRoot() + L"\\Optimizer\\session_history.txt";
    if (util::ReadFile(src, text)) {
        std::string mark = optimizerImport;                   // "yyyy-MM-dd HH:mm:ss"
        if (mark.empty()) {
            // sessions already brought in with a history-import.csv count as imported
            LoadHistory();
            for (auto& s : history)
                if (s.version == "imported" && s.date.size() == 16 && s.date + ":59" > mark) mark = s.date + ":59";
        }
        std::map<std::string, std::string> exeMap;
        for (auto& p : profiles) for (auto& e : p.exes) exeMap[util::Lower(e)] = p.name;
        std::istringstream in(text);
        std::string line, rows, newest = mark;
        int n = 0;
        while (std::getline(in, line)) {
            // [Mon Sep  1 14:03:22 2025] | game.exe | 1h 2m 3s
            if (line.empty() || line[0] != '[') continue;
            size_t close = line.find(']');
            if (close == std::string::npos) continue;
            auto parts = util::Split(line.substr(close + 1), '|');
            if (parts.size() < 2) continue;
            std::string when = ParseCtime(line.substr(1, close - 1));
            if (when.empty() || when <= mark) continue;
            std::string exe = parts[0], dur = parts[1];
            auto unit = [&](char u) -> int {   // number right before the unit letter ("1h 2m 3s")
                size_t q = dur.find(u);
                if (q == std::string::npos) return 0;
                size_t s = q;
                while (s > 0 && (isdigit((unsigned char)dur[s - 1]) || dur[s - 1] == ' ')) s--;
                return atoi(dur.substr(s, q - s).c_str());
            };
            double mins = 60.0 * unit('h') + unit('m') + unit('s') / 60.0;
            if (mins < 1) continue;
            std::string bare = util::StripExe(exe), key = util::Lower(bare);
            std::string game = exeMap.count(key) ? exeMap[key] : bare;
            char m[32]; snprintf(m, sizeof(m), "%.1f", mins);
            rows += CsvQuote(when.substr(0, 16)) + "," + CsvQuote(game) + "," + CsvQuote(m) + ",\"\",\"\",\"Optimizer\"\r\n";
            if (when > newest) newest = when;
            n++;
        }
        if (!rows.empty()) { CreateDirectoryW(DataDir().c_str(), nullptr); AppendCsv(HistoryPath(), rows); }
        if (newest > mark || optimizerImport.empty()) {
            if (newest.empty()) {
                time_t t = time(nullptr); tm lt; localtime_s(&lt, &t);
                char b[32]; strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &lt);
                newest = b;
            }
            optimizerImport = newest;
            SaveConfig();
        }
        added += n;
    }
    if (added) LoadHistory();
    return added;
}
