#include "optimizer.h"
#include "tweakset.h"
#include "tweaks.h"
#include "util.h"
#include <algorithm>
#include <chrono>
#include <cstdio>

namespace {

// Never closed, lowered or pinned: Windows itself, anti-cheat services and the VR runtime
const std::vector<std::string> kProtected = {
    "explorer", "dwm", "csrss", "winlogon", "lsass", "services", "svchost", "System", "smss", "wininit", "powershell", "conhost", "audiodg",
    "vgc", "vgtray", "BEService", "BEService_x64", "EasyAntiCheat", "EasyAntiCheat_EOS", "FACEIT", "faceitservice", "OVRServer_x64",
    "ProjectOptM",
};

const std::vector<std::pair<std::string, std::vector<std::string>>> kLauncherGroups = {
    { "steam",     { "steam", "steamwebhelper", "steamservice" } },
    { "epic",      { "EpicGamesLauncher", "EpicWebHelper" } },
    { "riot",      { "RiotClientServices", "RiotClientUx", "RiotClientUxRender" } },
    { "battlenet", { "Battle.net", "Agent" } },
    { "ea",        { "EADesktop", "EABackgroundService" } },
    { "oculus",    { "OculusClient", "OVRRedir", "OVRServiceLauncher" } },
    { "xbox",      { "XboxPcApp", "XboxPcAppFT", "XboxApp", "XboxGameOverlay" } },
    { "ubisoft",   { "upc", "UbisoftConnect", "UplayWebCore" } },
    { "gog",       { "GalaxyClient", "GalaxyClientService", "GOG Galaxy Notifications Renderer" } },
};

const std::vector<std::string> kUpdateServices = { "UsoSvc", "wuauserv", "DoSvc" };

uint64_t Now() { return GetTickCount64(); }

bool IsLauncher(const std::string& name) {
    for (auto& [g, list] : kLauncherGroups) if (util::Contains(list, name)) return true;
    return false;
}

void AddUnique(std::vector<std::string>& v, const std::string& s) { if (!util::Contains(v, s)) v.push_back(s); }

}  // namespace

void Optimizer::Init(const SystemInfo* sys, AppData* data, LogFn log) {
    sys_ = sys;
    data_ = data;
    log_ = std::move(log);
    RefreshTweaks();
}

void Optimizer::RefreshTweaks() {
    if (!active_) on_ = tweakset::Active(*data_, *sys_);
}

// ============================================================ descriptions
std::vector<std::string> Optimizer::VendorApps(const std::string& vendor) const {
    if (vendor == "AMD") return { "RadeonSoftware", "AMDRSServ", "AMDRSSrcExt", "amdow", "cncmd" };
    if (vendor == "NVIDIA") return { "NVIDIA app", "NVIDIA Overlay", "NVIDIA Share", "NVIDIA Web Helper" };
    if (vendor == "Intel") return { "ArcControl", "IntelGraphicsSoftware", "IGCC" };
    return {};
}

std::vector<std::string> Optimizer::BackgroundList() const {
    std::vector<std::string> l;
    for (auto& b : data_->settings.background) AddUnique(l, b);
    if (data_->settings.vendorApps && On("vendor_apps"))
        for (auto& g : sys_->gpus) for (auto& a : VendorApps(g.vendor)) AddUnique(l, a);
    return l;
}

uint64_t Optimizer::BackgroundMask() const {
    if (data_->settings.backgroundCores == "off" || !sys_->canPin || !On("bg_affinity")) return 0;
    if ((sys_->layout == CpuLayout::Hybrid || sys_->layout == CpuLayout::DualX3D) && sys_->otherMask) return sys_->otherMask;
    return 0;
}

std::string Optimizer::CoreMode(const GameProfile& p) const {
    std::string m = p.cores;
    if (m == "Other" && sys_->layout != CpuLayout::DualX3D) m = "Best";
    if (!sys_->canPin || !On("pinning")) m = "All";
    return m;
}

bool Optimizer::SoftPin(const GameProfile& p) const { return p.softPin || On("soft_pin"); }

int Optimizer::PurgeMinutes(const GameProfile& p) const {
    if (p.ramCleanupMins <= 0 || sys_->ramGB >= 48) return 0;
    if (sys_->ramGB <= 16) return std::max(5, p.ramCleanupMins / 2);
    return p.ramCleanupMins;
}

// Plan / Summary describe a game with its own tweaks, not the ones currently loaded
namespace {
struct GameTweaks {
    std::set<std::string>& on;
    std::set<std::string> saved;
    GameTweaks(std::set<std::string>& o, std::set<std::string> game) : on(o), saved(std::move(o)) { on = std::move(game); }
    ~GameTweaks() { on = std::move(saved); }
};
}  // namespace

std::string Optimizer::Summary(const GameProfile& p) const {
    GameTweaks use(on_, tweakset::Active(*data_, *sys_, &p));
    std::string s;
    if (p.antiCheat) {
        s = "Anti-cheat safe mode";
        if (!p.launchPriority.empty()) s += "  |  starts at " + PriorityLabel(p.launchPriority) + " priority";
        else s += " (system tweaks only)";
    } else {
        std::string m = CoreMode(p);
        std::string cores = m == "Best" ? sys_->BestLabel() : m == "Other" ? sys_->OtherLabel() : "All cores";
        if (!cores.empty()) cores[0] = (char)toupper((unsigned char)cores[0]);
        if (m != "All" && SoftPin(p)) cores += " (soft)";
        s = cores + "  |  " + PriorityLabel(p.priority) + " priority";
    }
    int pm = PurgeMinutes(p);
    if (pm > 0) s += "  |  RAM cleanup every " + std::to_string(pm) + "m";
    if (util::Contains(p.boost, "OVRServer_x64")) s += "  |  VR runtime boost";
    else if (!p.boost.empty()) s += "  |  helper boost";
    if (!p.tweaks.empty()) s += "  |  " + tweakset::PresetFor(*data_, &p) + " tweaks";
    return s;
}

std::vector<std::string> Optimizer::Plan(const GameProfile& p) const {
    GameTweaks use(on_, tweakset::Active(*data_, *sys_, &p));
    std::vector<std::string> l;
    const IniSettings& set = data_->settings;
    if (p.antiCheat) {
        l.push_back("Game process: not touched (anti-cheat safe mode)");
        if (!p.launchPriority.empty()) l.push_back("Launch priority: " + p.launchPriority + " (applied by Windows itself at start)");
    } else {
        l.push_back(On("priority") ? "Game priority: " + p.priority : "Game priority: unchanged");
        std::string m = CoreMode(p);
        if (m == "All") l.push_back("CPU cores: all (" + sys_->LayoutText() + " - no pinning needed)");
        else {
            uint64_t mask = m == "Other" ? sys_->otherMask : sys_->bestMask;
            l.push_back(std::string(SoftPin(p) ? "CPU cores: prefers " : "CPU cores: pinned to ") + (m == "Other" ? sys_->OtherLabel() : sys_->BestLabel()) +
                        " (CPU " + SystemInfo::MaskText(mask) + ")" + (SoftPin(p) ? " - soft, with CPU sets" : ""));
        }
        if (p.ecoQosOff && On("ecoqos")) l.push_back("Efficiency mode: blocked for the game");
        if (sys_->gpus.size() >= 2 && set.forceGpu && On("gpu_lock")) l.push_back("GPU: locked to " + sys_->gpus[0].name);
    }
    auto bg = BackgroundList();
    uint64_t bgm = BackgroundMask();
    if (bgm) l.push_back("Background apps: moved to " + sys_->OtherLabel() + " (CPU " + SystemInfo::MaskText(bgm) + ")");
    if (!bg.empty() && On("bg_priority")) {
        std::vector<std::string> first(bg.begin(), bg.begin() + std::min<size_t>(4, bg.size()));
        l.push_back("Low priority: " + std::to_string(bg.size()) + " background apps (" + util::Join(first, ", ") + (bg.size() > 4 ? ", ..." : "") + ")");
    }
    if (!bg.empty() && On("bg_eco")) l.push_back("Background apps: efficiency mode");
    if (On("power_mode")) l.push_back("Windows power mode: Best performance");
    if (!p.boost.empty()) l.push_back("High priority: " + util::Join(p.boost, ", "));
    std::vector<std::string> closeList = set.close;
    closeList.insert(closeList.end(), p.close.begin(), p.close.end());
    if (!closeList.empty() && On("close_apps")) l.push_back("Closed at start: " + util::Join(closeList, ", ") + (set.reopenClosed ? " (reopened after)" : ""));
    if (!p.keep.empty()) l.push_back("Never closed for this game: " + util::Join(p.keep, ", "));
    if (On("cloud_sync")) l.push_back("Cloud sync paused: " + util::Join(CloudApps(), ", ") + " (opened again after)");
    std::vector<std::string> paused;
    for (auto& s : PauseList()) {
        bool wu = s == "UsoSvc" || s == "wuauserv" || s == "DoSvc";
        AddUnique(paused, wu ? "Windows Update" : s);
    }
    if (!paused.empty()) l.push_back("Paused while playing: " + util::Join(paused, ", "));
    if (!On("power_plan") || set.powerPlan == "off") l.push_back("Power plan: unchanged");
    else if (set.powerPlan == "auto" && sys_->layout == CpuLayout::DualX3D) l.push_back("Power plan: unchanged (X3D needs Balanced)");
    else l.push_back("Power plan: High performance while playing");
    int pm = On("ram_cleaner") ? PurgeMinutes(p) : 0;
    if (pm > 0) l.push_back("RAM cleanup: at launch, then every " + std::to_string(pm) + " min");
    else if (On("standby")) l.push_back("RAM cleanup: once at launch");
    std::vector<std::string> extra;
    for (const char* id : { "timer", "io_priority", "mmcss", "explorer", "smt", "unpark", "cstate", "maxboost", "ws_trim",
                            "dvr", "visual", "fso", "audio", "defender", "dwm", "game_mode", "maintenance", "transparency",
                            "keep_awake", "pcie_aspm", "mouse_accel", "hotkeys" })
        if (On(id)) extra.push_back(tweakset::Find(id)->name);
    if (!extra.empty()) l.push_back("Tweaks (" + tweakset::PresetFor(*data_, &p) + "): " + util::Join(extra, ", "));
    if (data_->fpsOn) l.push_back("FPS graph: on");
    return l;
}

// ============================================================ detection loop
bool Optimizer::Running(const GameProfile& p, const ProcessList& procs) const {
    for (auto& e : p.exes) if (procs.Has(e)) return true;
    return false;
}

double Optimizer::SessionMinutes() const { return active_ ? (Now() - sessionStart_) / 60000.0 : 0; }

void Optimizer::Tick(const ProcessList& procs) {
    CheckPurge();
    if (!data_->autoOptimize) return;
    if (!active_) {
        for (auto& p : data_->profiles)
            if (Running(p, procs)) {
                GameProfile g = p;
                if (onPrepare) onPrepare(g);
                Enable(g, procs);
                break;
            }
    } else if (!Running(*active_, procs)) {
        EndSession(true);
    } else {
        Upkeep(procs);
    }
}

void Optimizer::Enable(const GameProfile& p, const ProcessList& procs) {
    on_ = tweakset::Active(*data_, *sys_, &p);   // this session's tweaks (the game's own, if it has them)
    active_ = p;
    log_(">> " + p.name + " detected (tweaks: " + tweakset::PresetFor(*data_, &p) + (p.tweaks.empty() ? "" : ", set for this game") + ")");
    sessionStart_ = Now();
    cleanups_ = 0;
    if (p.antiCheat) {
        log_("  Anti-cheat game: process left untouched, system tweaks only");
        if (!p.launchPriority.empty()) log_("  Started at " + p.launchPriority + " priority by Windows (launch priority)");
    }
    if (On("close_apps")) {
        std::vector<std::string> closeList = data_->settings.close;
        closeList.insert(closeList.end(), p.close.begin(), p.close.end());
        CloseApps(closeList, p.keep, procs);
    }
    if (On("cloud_sync")) PauseCloudSync(procs);

    auto bg = BackgroundList();
    if (On("bg_priority")) {
        SetPriority(bg, BELOW_NORMAL_PRIORITY_CLASS, procs);
        log_("  Background apps set to low priority");
    }
    if (MoveBackground(bg, procs) > 0)
        log_("  Background apps moved to " + sys_->OtherLabel() + " (CPU " + SystemInfo::MaskText(BackgroundMask()) + ")");
    if (On("bg_eco") && EcoBackground(bg, procs) > 0) log_("  Background apps in efficiency mode");
    if (!p.boost.empty()) { SetPriority(p.boost, HIGH_PRIORITY_CLASS, procs); log_("  Boosted: " + util::Join(p.boost, ", ")); }

    auto pause = PauseList();
    if (!pause.empty()) {
        PauseServices();
        std::vector<std::string> what;
        for (auto& s : pause) {
            bool wu = s == "UsoSvc" || s == "wuauserv" || s == "DoSvc";
            AddUnique(what, wu ? "Windows Update" : s);
        }
        log_("  Paused: " + util::Join(what, ", "));
    }
    SetGamingPowerPlan();
    ApplySessionTweaks(p, procs);
    for (auto& e : p.exes) for (DWORD pid : procs.Find(e)) TuneGame(pid, procs.NameOf(pid), p);
    if ((On("ram_cleaner") && PurgeMinutes(p) > 0) || On("standby")) ClearStandby();
    lastPurge_ = Now();
    if (onSessionStart) onSessionStart(p);
}

void Optimizer::Upkeep(const ProcessList& procs) {
    const GameProfile& a = *active_;
    for (auto& e : a.exes)
        for (DWORD pid : procs.Find(e)) {
            ProcKey k = { pid, proc::CreationTime(pid) };
            if (!tuned_.count(k)) TuneGame(pid, procs.NameOf(pid), a);
        }
    auto bg = BackgroundList();
    if (On("bg_priority")) SetPriority(bg, BELOW_NORMAL_PRIORITY_CLASS, procs);   // apps opened since the game started
    MoveBackground(bg, procs);
    if (On("bg_eco")) EcoBackground(bg, procs);
    if (!a.boost.empty()) SetPriority(a.boost, HIGH_PRIORITY_CLASS, procs);
    if (On("explorer")) ForcePriority("explorer", BELOW_NORMAL_PRIORITY_CLASS, procs);
    if (!PauseList().empty()) PauseServices();                 // Windows restarts update services on its own
    int pm = On("ram_cleaner") ? PurgeMinutes(a) : 0;
    if (pm > 0 && Now() - lastPurge_ >= (uint64_t)pm * 60000) { ClearStandby(); lastPurge_ = Now(); }
    if (trimAt_ && Now() >= trimAt_) {                         // game has loaded: trim background apps once
        trimAt_ = 0;
        int n = 0;
        DWORD self = GetCurrentProcessId();
        for (auto& name : bg) for (DWORD pid : procs.Find(name)) if (pid != self && proc::TrimWorkingSet(pid)) n++;
        if (n) log_("  Trimmed memory of " + std::to_string(n) + " background processes");
    }
}

void Optimizer::TuneGame(DWORD pid, const std::string& name, const GameProfile& p) {
    ProcKey key = { pid, proc::CreationTime(pid) };
    tuned_.insert(key);
    if (p.antiCheat) return;
    // remember the game's own settings too, so they're put back if the session ends while it's
    // still running (closing the app, Panic, auto-optimize off)
    DWORD origPrio;
    if (!savedPriority_.count(key) && proc::GetPriority(pid, origPrio)) savedPriority_[key] = origPrio;
    uint64_t origAff;
    if (!savedAffinity_.count(key) && proc::GetAffinity(pid, origAff)) savedAffinity_[key] = origAff;
    cpuSetKeys_.insert(key);
    std::vector<std::string> parts;
    if (On("priority"))
        parts.push_back(proc::SetPriority(pid, proc::PriorityClassOf(p.priority)) ? "priority " + p.priority : "priority unchanged");
    std::string mode = CoreMode(p);
    uint64_t gameMask = sys_->allMask;
    if (sys_->canPin && mode != "All") {
        uint64_t mask = mode == "Other" ? sys_->otherMask : sys_->bestMask;
        std::string label = mode == "Other" ? sys_->OtherLabel() : sys_->BestLabel();
        if (SoftPin(p)) {   // steer with CPU sets (also how SMT scheduling is applied)
            bool ok = proc::SetCpuSets(pid, mask, On("smt"));
            parts.push_back(ok ? "prefers " + label + " (CPU " + SystemInfo::MaskText(mask) + ", soft" + (On("smt") ? ", one thread per core)" : ")")
                               : "core pinning unchanged");
        } else {
            bool ok = proc::SetAffinity(pid, mask);
            if (ok) gameMask = mask;
            parts.push_back(ok ? "pinned to " + label + " (CPU " + SystemInfo::MaskText(mask) + ")" : "core pinning unchanged");
        }
    }
    if (On("smt") && !(sys_->canPin && mode != "All" && SoftPin(p)) && proc::SetCpuSets(pid, gameMask, true)) parts.push_back("one thread per core");
    if (On("io_priority") && proc::SetIoPriorityHigh(pid)) parts.push_back("high I/O priority");
    if (On("ecoqos") && p.ecoQosOff && proc::SetEcoQosOff(pid, true)) { parts.push_back("efficiency mode off"); ecoPids_.push_back(pid); }
    if (!parts.empty()) log_("  " + name + ": " + util::Join(parts, ", "));
    std::wstring path = proc::ImagePath(pid);
    if (On("gpu_lock") && data_->settings.forceGpu && sys_->gpus.size() >= 2 && !path.empty()) {
        std::string before = util::Narrow(tweaks::GpuPreference(path));   // yours, put back on exit
        if (tweaks::PreferDedicatedGpu(path)) {
            std::string np = util::Narrow(path);
            bool known = false;
            for (auto& g : data_->gpuManaged) if (g.substr(0, g.find('|')) == np) known = true;
            if (!known) { data_->gpuManaged.push_back(np + "|" + before); data_->SaveConfig(); }
            log_("  " + name + ": locked to " + sys_->gpus[0].name + " (from next launch)");
        }
    }
    if (On("fso") && !path.empty() && tweaks::SetFullscreenOptimizationsOff(path, true)) {
        std::string np = util::Narrow(path);
        if (!util::Contains(data_->fsoManaged, np)) { data_->fsoManaged.push_back(np); data_->SaveConfig(); }
        log_("  " + name + ": fullscreen optimizations off (from next launch)");
    }
}

// ============================================================ individual tweaks
void Optimizer::SetPriority(const std::vector<std::string>& names, DWORD cls, const ProcessList& procs) {
    DWORD self = GetCurrentProcessId();
    bool lowering = cls == BELOW_NORMAL_PRIORITY_CLASS || cls == IDLE_PRIORITY_CLASS;
    for (auto& n : names) {
        if (lowering && util::Contains(kProtected, n)) continue;   // boosting the VR runtime is fine, lowering system apps isn't
        for (DWORD pid : procs.Find(n)) {
            if (pid == self) continue;
            ProcKey k = { pid, proc::CreationTime(pid) };
            if (!k.created || savedPriority_.count(k)) continue;
            DWORD orig;
            if (proc::GetPriority(pid, orig) && proc::SetPriority(pid, cls)) savedPriority_[k] = orig;
        }
    }
}

int Optimizer::MoveBackground(const std::vector<std::string>& names, const ProcessList& procs) {
    uint64_t mask = BackgroundMask();
    if (!mask) return 0;
    int n = 0;
    DWORD self = GetCurrentProcessId();
    for (auto& name : names) {
        if (util::Contains(kProtected, name)) continue;
        for (DWORD pid : procs.Find(name)) {
            if (pid == self) continue;
            ProcKey k = { pid, proc::CreationTime(pid) };
            if (!k.created || savedAffinity_.count(k)) continue;
            uint64_t orig;
            if (proc::GetAffinity(pid, orig) && proc::SetAffinity(pid, mask)) { savedAffinity_[k] = orig; n++; }
        }
    }
    return n;
}

int Optimizer::EcoBackground(const std::vector<std::string>& names, const ProcessList& procs) {
    int n = 0;
    DWORD self = GetCurrentProcessId();
    for (auto& name : names) {
        if (util::Contains(kProtected, name)) continue;
        for (DWORD pid : procs.Find(name)) {
            if (pid == self) continue;
            ProcKey k = { pid, proc::CreationTime(pid) };
            if (!k.created || ecoBg_.count(k)) continue;
            if (proc::SetEcoQosOn(pid, true)) { ecoBg_.insert(k); n++; }
        }
    }
    return n;
}

std::vector<std::string> Optimizer::Expand(const std::vector<std::string>& names) const {
    std::vector<std::string> out;
    for (auto& n : names) {
        bool group = false;
        for (auto& [g, list] : kLauncherGroups)
            if (util::Lower(n) == g) { for (auto& x : list) AddUnique(out, x); group = true; }
        if (!group) AddUnique(out, n);
    }
    return out;
}

void Optimizer::CloseApps(const std::vector<std::string>& names, const std::vector<std::string>& keep, const ProcessList& procs) {
    std::vector<std::string> gameNames;
    for (auto& p : data_->profiles) gameNames.insert(gameNames.end(), p.exes.begin(), p.exes.end());
    auto keepNames = Expand(keep);
    std::vector<std::string> closed;
    DWORD self = GetCurrentProcessId();
    for (auto& n : Expand(names)) {
        if (util::Contains(kProtected, n) || util::Contains(gameNames, n) || util::Contains(keepNames, n)) continue;
        for (DWORD pid : procs.Find(n)) {
            if (pid == self) continue;
            std::wstring path = proc::ImagePath(pid);
            std::string realName = procs.NameOf(pid);
            if (!proc::Close(pid)) continue;
            AddUnique(closed, realName);
            if (!path.empty() && !IsLauncher(realName) &&
                std::none_of(reopenPaths_.begin(), reopenPaths_.end(), [&](auto& p) { return _wcsicmp(p.c_str(), path.c_str()) == 0; }))
                reopenPaths_.push_back(path);
        }
    }
    if (!closed.empty()) log_("  Closed: " + util::Join(closed, ", "));
}

// Cloud sync apps: closed for the session, and each one's path is saved as a backup ("O|path") so it's
// opened again when the game closes - or on the next start if we crash
std::vector<std::string> Optimizer::CloudApps() const {
    std::wstring test = util::EnvVar(L"OPTM_TEST_CLOUD");   // test copies: stand-ins, never your real OneDrive
    if (!util::EnvVar(L"OPTM_DATA_DIR").empty()) return test.empty() ? std::vector<std::string>{} : util::NameList(util::Narrow(test));
    return { "OneDrive", "Dropbox", "GoogleDriveFS", "MEGAsync" };
}

void Optimizer::PauseCloudSync(const ProcessList& procs) {
    std::vector<std::string> closed;
    DWORD self = GetCurrentProcessId();
    for (auto& n : CloudApps()) {
        std::set<std::string> seen;   // one backup per exe, however many processes it runs
        for (DWORD pid : procs.Find(n)) {
            if (pid == self) continue;
            std::wstring path = proc::ImagePath(pid);
            if (path.empty()) continue;
            if (seen.insert(util::Lower(util::Narrow(path))).second) Backup("O|" + util::Narrow(path));
            if (proc::Close(pid)) AddUnique(closed, procs.NameOf(pid));
        }
    }
    if (!closed.empty()) log_("  Cloud sync paused: " + util::Join(closed, ", ") + " (opened again after)");
}

void Optimizer::ReopenClosedApps() {
    if (data_->settings.reopenClosed && !reopenPaths_.empty()) {
        ProcessList now;
        now.Refresh();
        std::vector<std::wstring> running;
        for (DWORD pid : now.All()) { auto p = proc::ImagePath(pid); if (!p.empty()) running.push_back(p); }
        std::vector<std::string> names;
        for (auto& path : reopenPaths_) {
            bool open = std::any_of(running.begin(), running.end(), [&](auto& r) { return _wcsicmp(r.c_str(), path.c_str()) == 0; });
            if (!open) util::OpenAsUser(path);
            std::wstring file = path.substr(path.find_last_of(L"\\/") + 1);
            if (file.size() > 4) file.resize(file.size() - 4);
            names.push_back(util::Narrow(file));
        }
        log_("  Reopened: " + util::Join(names, ", "));
    }
    reopenPaths_.clear();
}

std::vector<std::string> Optimizer::PauseList() const {
    // Service suspend = your pause_services list + telemetry; Prefetch off = SysMain; plus Windows Update
    std::vector<std::string> l;
    if (data_->settings.pauseUpdates && On("updates")) for (auto& s : kUpdateServices) AddUnique(l, s);
    if (On("services")) {
        for (auto& s : data_->settings.pauseServices) if (util::Lower(s) != "sysmain") AddUnique(l, s);
        AddUnique(l, "DiagTrack");
    }
    if (On("prefetch")) AddUnique(l, "SysMain");
    return l;
}

void Optimizer::PauseServices() {
    bool changed = false;
    for (auto& n : PauseList()) {
        if (tweaks::ServiceRunning(n) && tweaks::StopService(n) && !util::Contains(data_->pausedServices, n)) {
            data_->pausedServices.push_back(n);
            changed = true;
        }
    }
    if (changed) data_->SaveConfig();   // remembered so a crash doesn't leave them off
}

void Optimizer::ResumeServices() {
    if (data_->pausedServices.empty()) return;
    for (auto& n : data_->pausedServices) tweaks::StartService(n);
    data_->pausedServices.clear();
    data_->SaveConfig();
}

void Optimizer::SetGamingPowerPlan() {
    const std::string& mode = data_->settings.powerPlan;
    if (!On("power_plan") || mode == "off" || (mode == "auto" && sys_->layout == CpuLayout::DualX3D)) return;
    std::string target;
    for (const char* g : { tweaks::kHighPerformance, tweaks::kUltimate })
        if (target.empty() && tweaks::PlanExists(g)) target = g;
    std::string cur = tweaks::ActivePlan();
    if (target.empty() || cur.empty() || cur == target) return;
    data_->restorePlan = cur;
    data_->SaveConfig();
    if (tweaks::SetPlan(target)) log_("  Power plan: High performance (until the game closes)");
}

void Optimizer::RestorePowerPlan() {
    if (data_->restorePlan.empty()) return;
    tweaks::SetPlan(data_->restorePlan);
    data_->restorePlan.clear();
    data_->SaveConfig();
}

// Purging standby memory can take a few seconds on big RAM, so it runs off the UI thread
void Optimizer::ClearStandby() {
    if (purge_.valid()) return;   // one at a time
    purge_ = std::async(std::launch::async, [] { return proc::PurgeStandbyList(); });
}

void Optimizer::CheckPurge() {
    if (!purge_.valid() || purge_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    uint32_t r = purge_.get();
    if (r == 0) { log_("  Cleared standby RAM"); cleanups_++; return; }
    char b[64]; snprintf(b, sizeof(b), "  Standby RAM clear failed (0x%08X)", r);
    log_(b);
}

// ============================================================ restore
void Optimizer::RestoreAll() {
    for (DWORD pid : ecoPids_) proc::SetEcoQosOff(pid, false);
    ecoPids_.clear();
    for (auto& k : ecoBg_) if (proc::Alive(k)) proc::SetEcoQosOn(k.pid, false);
    ecoBg_.clear();
    for (auto& k : cpuSetKeys_) if (proc::Alive(k)) proc::ClearCpuSets(k.pid);   // game still running: all CPUs again
    cpuSetKeys_.clear();
    RevertSessionTweaks();   // before the power plan goes back - some of these live in the plan
    ResumeServices();
    RestorePowerPlan();
    ReopenClosedApps();
    for (auto& [k, cls] : savedPriority_) if (proc::Alive(k)) proc::SetPriority(k.pid, cls);
    savedPriority_.clear();
    for (auto& [k, mask] : savedAffinity_) if (proc::Alive(k)) proc::SetAffinity(k.pid, mask);
    savedAffinity_.clear();
    tuned_.clear();
}

void Optimizer::EndSession(bool gameClosed) {
    if (purge_.valid()) { purge_.wait(); CheckPurge(); }   // count it in this session
    if (!active_) { RestoreAll(); return; }
    double mins = SessionMinutes();
    GameProfile p = *active_;
    RestoreAll();
    active_.reset();
    if (onSessionEnd) onSessionEnd(p, mins, gameClosed);
}

void Optimizer::Panic() {
    EndSession();
    data_->autoOptimize = false;
    data_->SaveConfig();
    log_("!! PANIC - everything restored and auto-optimize paused. Turn auto-optimize back on to resume.");
}

void Optimizer::SetAuto(bool on) {
    data_->autoOptimize = on;
    data_->SaveConfig();
    if (!on && active_) {
        std::string name = active_->name;
        EndSession();
        log_("Auto-optimize off - " + name + " restored to normal");
    } else {
        log_(on ? "Auto-optimize on" : "Auto-optimize off");
    }
}

// ============================================================ Tweaks page session tweaks
bool Optimizer::Backup(const std::string& b) {
    if (b.empty()) return false;   // nothing was changed
    data_->tweakBackups.push_back(b);
    data_->SaveConfig();   // saved right away so a crash can be undone on the next start
    return true;
}

bool Optimizer::ForcePriority(const std::string& name, DWORD cls, const ProcessList& procs) {
    bool any = false;
    for (DWORD pid : procs.Find(name)) {
        ProcKey k = { pid, proc::CreationTime(pid) };
        if (!k.created) continue;
        if (savedPriority_.count(k)) { any = true; continue; }
        DWORD orig;
        if (proc::GetPriority(pid, orig) && proc::SetPriority(pid, cls)) {
            savedPriority_[k] = orig;
            any = true;
            Backup("R|" + name + "|" + std::to_string(orig));   // system processes: put back even after a crash
        }
    }
    return any;
}

void Optimizer::ApplySessionTweaks(const GameProfile& p, const ProcessList& procs) {
    std::vector<std::string> done;
    if (On("timer") && tweaks::SetTimerResolution(true)) {
        timerOn_ = true;
        done.push_back("0.5 ms timer");
        if (tweaks::EnableGlobalTimerRequests()) log_("  Timer resolution: restart your PC once so Windows 11 applies it to games too");
    }
    if (On("mmcss")) {
        const std::wstring k = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";
        Backup(tweaks::SetRegDword(HKEY_LOCAL_MACHINE, k, L"GPU Priority", 8));
        Backup(tweaks::SetRegDword(HKEY_LOCAL_MACHINE, k, L"Priority", 6));
        Backup(tweaks::SetRegString(HKEY_LOCAL_MACHINE, k, L"Scheduling Category", L"High"));
        Backup(tweaks::SetRegString(HKEY_LOCAL_MACHINE, k, L"SFIO Priority", L"High"));
        // only claim it if it's really in place (needs admin; Windows' default is "Medium")
        if (util::RegString(HKEY_LOCAL_MACHINE, k.c_str(), L"Scheduling Category") == "High") done.push_back("MMCSS Games profile");
    }
    if (On("explorer") && ForcePriority("explorer", BELOW_NORMAL_PRIORITY_CLASS, procs)) done.push_back("explorer low");
    if (On("audio") && ForcePriority("audiodg", HIGH_PRIORITY_CLASS, procs)) done.push_back("audio engine High");
    if (On("dwm") && ForcePriority("dwm", HIGH_PRIORITY_CLASS, procs)) done.push_back("DWM High");
    if (On("visual")) {
        std::string b = tweaks::SetVisualEffects(false);
        Backup(b);
        done.push_back(b.empty() ? "animations already off" : "animations off");
    }
    if (On("dvr")) {
        std::string a = tweaks::SetRegDword(HKEY_CURRENT_USER, L"System\\GameConfigStore", L"GameDVR_Enabled", 0);
        std::string b = tweaks::SetRegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR", L"AppCaptureEnabled", 0);
        Backup(a); Backup(b);
        done.push_back("Game DVR capture off");
    }
    auto power = [&](const GUID& setting, DWORD value) { std::string b = tweaks::SetPowerValue(tweaks::kSubProcessor, setting, value); Backup(b); return !b.empty(); };
    if (On("unpark") && power(tweaks::kCoreParkingMin, 100)) done.push_back("cores unparked");
    if (On("cstate") && power(tweaks::kIdleDisable, 1)) done.push_back("C-states off");
    if (On("maxboost")) {
        bool a = power(tweaks::kBoostMode, 2), b = power(tweaks::kEnergyPref, 0);
        if (a || b) done.push_back("max boost");
    }
    if (On("defender")) {
        std::wstring folder;
        for (auto& e : p.exes) for (DWORD pid : procs.Find(e)) if (folder.empty()) folder = proc::ImagePath(pid);
        folder = folder.substr(0, folder.find_last_of(L"\\/"));
        if (!folder.empty()) {
            std::string b = tweaks::AddDefenderExclusion(folder);
            Backup(b);
            if (!b.empty()) done.push_back("Defender skips " + util::Narrow(folder));
        }
    }
    if (On("game_mode") && Backup(tweaks::SetRegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\GameBar", L"AutoGameModeEnabled", 1)))
        done.push_back("Game Mode on");
    if (On("maintenance")) {
        std::string b = tweaks::SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance", L"MaintenanceDisabled", 1);
        if (Backup(b)) done.push_back("automatic maintenance paused");
    }
    if (On("transparency") && Backup(tweaks::SetTransparency(false))) done.push_back("transparency off");
    if (On("keep_awake")) {   // this thread keeps the PC and screen awake until the session ends
        awake_ = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED) != 0;
        if (awake_) done.push_back("screen kept awake");
    }
    if (On("pcie_aspm") && Backup(tweaks::SetPowerValue(tweaks::kSubPciExpress, tweaks::kLinkStatePower, 0))) done.push_back("PCIe power saving off");
    if (On("mouse_accel") && Backup(tweaks::SetMouseAcceleration(false))) done.push_back("mouse acceleration off");
    if (On("hotkeys") && Backup(tweaks::SetAccessibilityHotkeys(false))) done.push_back("Sticky/Filter Keys shortcuts off");
    if (On("ws_trim")) trimAt_ = Now() + 60000;
    if (On("power_mode")) {
        std::string b = tweaks::SetPowerMode(true);
        Backup(b);
        if (!b.empty()) done.push_back("Best performance power mode");
    }
    if (!done.empty()) log_("  Tweaks: " + util::Join(done, ", "));
}

void Optimizer::RevertSessionTweaks() {
    if (timerOn_) { tweaks::SetTimerResolution(false); timerOn_ = false; }
    if (awake_) { SetThreadExecutionState(ES_CONTINUOUS); awake_ = false; }
    trimAt_ = 0;
    if (data_->tweakBackups.empty()) return;
    ProcessList procs;
    for (auto it = data_->tweakBackups.rbegin(); it != data_->tweakBackups.rend(); ++it) {
        if (it->rfind("O|", 0) == 0) {   // "O|exe path": an app we closed that must run again (cloud sync)
            std::wstring path = util::Widen(it->substr(2));
            bool running = false;
            if (procs.All().empty()) procs.Refresh();
            for (DWORD pid : procs.All()) if (_wcsicmp(proc::ImagePath(pid).c_str(), path.c_str()) == 0) { running = true; break; }
            if (!running && GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) util::OpenAsUser(path);
        } else if (it->rfind("R|", 0) == 0) {   // "R|name|priority class": a system process we re-prioritized
            auto f = util::Split(it->substr(2), '|');
            if (f.size() < 2) continue;
            if (procs.All().empty()) procs.Refresh();
            for (DWORD pid : procs.Find(f[0])) proc::SetPriority(pid, (DWORD)strtoul(f[1].c_str(), nullptr, 10));
        } else {
            tweaks::Restore(*it);
        }
    }
    data_->tweakBackups.clear();
    data_->SaveConfig();
}

// A test copy (its own data folder) must never change Windows-wide settings from its sandbox profiles -
// unless a test asks for it (OPTM_TEST_SYSTEM=1, used with a made-up test game)
bool Optimizer::SystemWide() const {
    return util::EnvVar(L"OPTM_DATA_DIR").empty() || !util::EnvVar(L"OPTM_TEST_SYSTEM").empty();
}

bool Optimizer::AnyGameUses(const std::string& id) const {
    if (tweakset::Active(*data_, *sys_).count(id)) return true;
    for (auto& p : data_->profiles) if (tweakset::Active(*data_, *sys_, &p).count(id)) return true;
    return false;
}

void Optimizer::SyncPerGameSettings() {
    if (!SystemWide()) return;
    if (!data_->fsoManaged.empty() && !AnyGameUses("fso")) {
        for (auto& path : data_->fsoManaged) tweaks::SetFullscreenOptimizationsOff(util::Widen(path), false);
        log_("Fullscreen optimizations turned back on for " + std::to_string(data_->fsoManaged.size()) + " game(s)");
        data_->fsoManaged.clear();
        data_->SaveConfig();
    }
    if (!data_->gpuManaged.empty() && (!AnyGameUses("gpu_lock") || !data_->settings.forceGpu)) {
        RestoreGpuPreferences();
        data_->gpuManaged.clear();
        data_->SaveConfig();
    }
}

void Optimizer::RestoreGpuPreferences() {
    for (auto& g : data_->gpuManaged) {
        size_t bar = g.find('|');
        tweaks::SetGpuPreference(util::Widen(g.substr(0, bar)), bar == std::string::npos ? L"" : util::Widen(g.substr(bar + 1)));
    }
    if (!data_->gpuManaged.empty()) log_("GPU preference put back for " + std::to_string(data_->gpuManaged.size()) + " game(s)");
}

// Exiting: the settings Windows applies at a game's next launch go back too (the lists are kept,
// so the next start sets them again)
void Optimizer::RevertOnExit() {
    if (!SystemWide()) return;
    std::vector<std::string> left;
    for (auto& exe : data_->ifeoManaged) {
        tweaks::RemoveLaunchPriority(exe);
        for (const char* pr : { "Normal", "AboveNormal", "High" })
            if (tweaks::HasLaunchPriority(exe, pr)) { left.push_back(exe); break; }   // no admin rights
    }
    size_t removed = data_->ifeoManaged.size() - left.size();
    if (removed) log_("Launch priority removed for " + std::to_string(removed) + " game(s)");
    if (!left.empty()) log_("Couldn't remove launch priority for " + util::Join(left, ", ") + " (needs admin)");
    data_->ifeoManaged = left;    // SyncLaunchPriority sets the rest again at start
    for (auto& path : data_->fsoManaged) tweaks::SetFullscreenOptimizationsOff(util::Widen(path), false);
    if (!data_->fsoManaged.empty()) log_("Fullscreen optimizations turned back on for " + std::to_string(data_->fsoManaged.size()) + " game(s)");
    RestoreGpuPreferences();
    data_->SaveConfig();
}

void Optimizer::ReapplyAtStart() {
    if (!SystemWide()) return;
    int n = 0;
    if (AnyGameUses("fso"))
        for (auto& path : data_->fsoManaged) if (tweaks::SetFullscreenOptimizationsOff(util::Widen(path), true)) n++;
    if (AnyGameUses("gpu_lock") && data_->settings.forceGpu && sys_->gpus.size() >= 2)
        for (auto& g : data_->gpuManaged) if (tweaks::PreferDedicatedGpu(util::Widen(g.substr(0, g.find('|'))))) n++;
    if (n) log_("Per-game Windows settings set again (" + std::to_string(n) + ")");
}

void Optimizer::RecoverLastRun() {
    if (!data_->tweakBackups.empty()) { RevertSessionTweaks(); log_("Undid tweaks left on by the last session"); }
    if (!data_->restorePlan.empty()) { RestorePowerPlan(); log_("Restored your power plan from last session"); }
    if (!data_->pausedServices.empty()) {
        std::string list = util::Join(data_->pausedServices, ", ");
        ResumeServices();
        log_("Restarted services left paused by the last session: " + list);
    }
}

void Optimizer::SyncLaunchPriority() {
    if (!SystemWide()) return;
    std::map<std::string, std::string> want;   // lower-case "game.exe" -> priority
    std::map<std::string, std::string> spelled;
    for (auto& p : data_->profiles)
        if (!p.launchPriority.empty())
            for (auto& e : p.exes) { want[util::Lower(e + ".exe")] = p.launchPriority; spelled[util::Lower(e + ".exe")] = e + ".exe"; }
    for (auto& exe : data_->ifeoManaged) {
        if (want.count(util::Lower(exe))) continue;
        tweaks::RemoveLaunchPriority(exe);
        log_("Launch priority removed: " + exe);
    }
    std::vector<std::string> managed;
    for (auto& [key, prio] : want) {
        const std::string& exe = spelled[key];
        managed.push_back(exe);
        if (tweaks::HasLaunchPriority(exe, prio)) continue;
        if (tweaks::SetLaunchPriority(exe, prio)) log_("Launch priority set: " + exe + " starts at " + prio);
        else log_("Couldn't set launch priority for " + exe);
    }
    data_->ifeoManaged = managed;
    data_->SaveConfig();
}
