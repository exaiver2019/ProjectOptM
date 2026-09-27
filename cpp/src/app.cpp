#include "app.h"
#include "autostart.h"
#include "imgui.h"
#include "json.h"
#include "tweaks.h"
#include "tweakset.h"
#include "util.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <commdlg.h>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <map>
#include <shlobj.h>

namespace {

// ---------------- colors ----------------
ImVec4 Hex(const std::string& h, float a = 1.0f) {
    unsigned v = 0x4F8BFF;
    if (h.size() == 7 && h[0] == '#') v = (unsigned)strtoul(h.c_str() + 1, nullptr, 16);
    return ImVec4(((v >> 16) & 255) / 255.f, ((v >> 8) & 255) / 255.f, (v & 255) / 255.f, a);
}
ImU32 U32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }
ImVec4 Lighten(ImVec4 c, float t) { return ImVec4(c.x + (1 - c.x) * t, c.y + (1 - c.y) * t, c.z + (1 - c.z) * t, c.w); }
ImVec4 Alpha(ImVec4 c, float a) { c.w = a; return c; }

struct Palette { const char* name; const char* bg; const char* card; const char* card2; const char* btn; const char* chip; const char* line; };
const Palette kBackgrounds[] = {
    { "Dark",       "#0F1115", "#181B22", "#12151B", "#262B36", "#222631", "#2A2F3A" },
    { "Darker",     "#0A0B0E", "#121419", "#0D0F13", "#1F222A", "#1A1D24", "#23262E" },
    { "OLED Black", "#000000", "#0B0B0D", "#050506", "#1A1A1E", "#141417", "#1E1E22" },
    { "Slate",      "#161A22", "#1F2430", "#1A1E27", "#2E3544", "#2A303D", "#343B4A" },
};
struct Corner { const char* name; float card, btn; };
const Corner kCorners[] = { { "Rounded", 10, 6 }, { "Soft", 5, 3 }, { "Sharp", 0, 0 } };
const std::pair<const char*, const char*> kAccents[] = {
    { "Blue", "#4F8BFF" }, { "Crimson", "#E5484D" }, { "Emerald", "#30C48D" }, { "Violet", "#8E6CFF" },
    { "Amber", "#F5A524" }, { "Cyan", "#22C3E6" }, { "Pink", "#F062A8" }, { "Mono", "#D0D4DC" },
};

const ImVec4 kText(0.91f, 0.92f, 0.94f, 1), kSub(0.54f, 0.56f, 0.63f, 1), kDim(0.42f, 0.44f, 0.50f, 1);
const ImVec4 kGreen = Hex("#3DDC84"), kAmber = Hex("#F5B942"), kIdle = Hex("#3A3F4B"), kGray = Hex("#6B7180");

ImVec4 g_accent, g_card, g_card2, g_btn, g_chip, g_line;
float g_cardR = 10, g_btnR = 6;

ImVec4 TextOn(const ImVec4& c) { return (c.x * 0.299f + c.y * 0.587f + c.z * 0.114f) > 0.62f ? Hex("#15171C") : ImVec4(1, 1, 1, 1); }

// UTF-8 for a Segoe icon code point (E700-F8FF)
std::string Icon(unsigned cp) {
    std::string s;
    s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F));
    return s;
}

std::string PrettyDate(const std::string& d) {   // "2026-09-26 03:10" -> "Sep 26, 3:10 AM"
    int y, mo, da, h, mi;
    if (sscanf(d.c_str(), "%d-%d-%d %d:%d", &y, &mo, &da, &h, &mi) != 5 || mo < 1 || mo > 12) return d;
    static const char* M[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
    char b[40]; snprintf(b, sizeof(b), "%s %d, %d:%02d %s", M[mo - 1], da, h % 12 ? h % 12 : 12, mi, h < 12 ? "AM" : "PM");
    return b;
}

std::string Signed(double v) { char b[16]; snprintf(b, sizeof(b), "%+d", (int)std::lround(v)); return b; }

ImVec4 VendorColor(const std::string& v) {
    if (v == "AMD") return Hex("#ED1C24");
    if (v == "NVIDIA") return Hex("#76B900");
    if (v == "Intel") return Hex("#0071C5");
    return Hex("#8A90A0");
}

int Msg(HWND owner, const std::string& text, const std::string& title, UINT flags) {
    return MessageBoxW(owner, util::Widen(text).c_str(), util::Widen(title).c_str(), flags);
}

uint64_t Ms() { return GetTickCount64(); }

std::string Plural(size_t n, const char* word) { return std::to_string(n) + " " + word + (n == 1 ? "" : "s"); }

}  // namespace

// ============================================================ setup
void App::Init(HWND hwnd, float dpiScale) {
    hwnd_ = hwnd;
    dpi_ = dpiScale;
    startMs_ = Ms();
    selfPath_ = util::SelfPath();   // before any rename: Windows reports the renamed path afterwards
    std::string cmd = util::Lower(util::Narrow(GetCommandLineW()));
    const char* pages[] = { "home", "games", "sessions", "overlay", "tweaks", "system", "activity", "settings" };
    for (int i = 0; i < PageCount; i++)
        if (cmd.find(std::string("--page ") + pages[i]) != std::string::npos) page_ = (Page)i;

    Log("Project OptM v" OPTM_VERSION " starting");
    sys_.Detect();
    data_.Load();
    int imported = data_.ImportOtherHistory();
    snprintf(hexBuf_, sizeof(hexBuf_), "%s", data_.theme.accent.c_str());
    s_ = dpi_ * Zoom();
    LoadFonts();
    ApplyTheme();

    opt_.Init(&sys_, &data_, [this](const std::string& l) { Log(l); });
    opt_.onSessionStart = [this](const GameProfile& p) { SessionStarted(p); };
    opt_.onSessionEnd = [this](const GameProfile& p, double m, bool closed) { SessionEnded(p, m, closed); };

    // tray icon
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = WM_APP_TRAY;
    nid_.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    wcscpy_s(nid_.szTip, L"Project OptM");
    trayAdded_ = Shell_NotifyIconW(NIM_ADD, &nid_) != 0;

    std::string gpus;
    if (!sys_.gpus.empty()) gpus = ", " + sys_.gpus[0].name;
    Log("Detected: " + sys_.cpuName + gpus + ", " + std::to_string(sys_.ramGB) + " GB RAM");
    if (sys_.canPin) Log("Games will be pinned to " + sys_.BestLabel() + " (CPU " + SystemInfo::MaskText(sys_.bestMask) + ")");
    else Log("CPU layout: " + sys_.LayoutText() + " - core pinning not needed");

    if (data_.settings.panicHotkey) {
        hotkey_ = RegisterHotKey(hwnd_, kHotkeyPanic, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_END) != 0;
        Log(hotkey_ ? "Panic hotkey ready: Ctrl+Alt+End undoes everything instantly"
                    : "Panic hotkey Ctrl+Alt+End is used by another app - use the tray menu instead");
    }
    overlay_.Create(GetModuleHandleW(nullptr));
    if (!sys_.gpus.empty()) sensors_.Start(sys_.gpus[0].luidLow, sys_.gpus[0].luidHigh, sys_.gpus[0].vramBytes);
    overlayHotkey_ = RegisterHotKey(hwnd_, kHotkeyOverlay, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'O') != 0;
    overlayDemo_ = cmd.find("--overlay-demo") != std::string::npos;
    RefreshChecks();
    int warns = (int)std::count_if(checks_.begin(), checks_.end(), [](const Check& c) { return c.state == Check::Warn; });
    if (warns) Log(std::to_string(warns) + " system check(s) need attention - click the outlined chips on the System page to fix");

    opt_.RecoverLastRun();
    opt_.SyncPerGameSettings();
    opt_.ReapplyAtStart();   // put back what the last exit removed (launch priority: SyncLaunchPriority below)
    if (util::EnvVar(L"OPTM_DATA_DIR").empty()) {   // (test copies never touch the real startup task)
        startWithWindows_ = autostart::Enabled();
        // the exe moved (or this is a new copy): keep the task pointing at the one you use
        if (startWithWindows_ && _wcsicmp(autostart::TaskCommand().c_str(), selfPath_.c_str()) != 0) {
            std::string err;
            if (autostart::Enable(selfPath_, err)) Log("Start with Windows now starts this copy: " + util::Narrow(selfPath_));
        }
    }
    detector_.Load();
    if (data_.profilesCreated) Log("Created your game profiles file with the default games");
    if (imported) Log("Imported " + std::to_string(imported) + " past sessions into your play history");
    std::vector<std::string> makers;
    for (auto& g : sys_.gpus) if (g.vendor != "Other" && !util::Contains(makers, g.vendor)) makers.push_back(g.vendor);
    Log("Hardware: " + sys_.cpuVendor + " CPU" + (makers.empty() ? "" : " + " + util::Join(makers, " + ") + " graphics") + " - maker-specific tweaks on");
    opt_.SyncLaunchPriority();
    Log(Plural(data_.profiles.size(), "game profile") + " loaded. Watching for games...");
    for (auto& w : data_.warnings) Log("  ! " + w);

    int argc = 0;
    if (LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc)) {   // --history <game>: open its history
        for (int i = 1; i + 1 < argc; i++) {
            if (wcscmp(argv[i], L"--history") == 0) historyGame_ = util::Narrow(argv[i + 1]);
            if (wcscmp(argv[i], L"--game-settings") == 0) OpenGameSettings(util::Narrow(argv[i + 1]));   // open a game's settings
            if (wcscmp(argv[i], L"--zoom-after") == 0) testZoom_ = _wtof(argv[i + 1]) / 100.0;   // change size 1s in
            if (wcscmp(argv[i], L"--scroll") == 0) testScroll_ = (float)_wtof(argv[i + 1]);       // scroll the page (screenshots)
            if (wcscmp(argv[i], L"--expand") == 0) expanded_.insert(util::Narrow(argv[i + 1]));   // open a tweak's details
            if (wcscmp(argv[i], L"--search") == 0) snprintf(gameSearch_, sizeof(gameSearch_), "%s", util::Narrow(argv[i + 1]).c_str());
            if (wcscmp(argv[i], L"--tour") == 0) TourGo(_wtoi(argv[i + 1]));   // open the tour at a step (screenshots)
        }
        LocalFree(argv);
    }
    if (!editGame_.empty() && cmd.find("--save-settings") != std::string::npos) SaveGameSettings();   // developer check: profiles.ini round-trip
    if (cmd.find("--feedback") != std::string::npos) { OpenFeedback(); fbPreview_ = cmd.find("--feedback-preview") != std::string::npos; }
    if (!util::EnvVar(L"OPTM_FEEDBACK_TEST").empty()) {   // developer check: build the link (logged, not opened)
        snprintf(fbTitle_, sizeof(fbTitle_), "%s", util::Narrow(util::EnvVar(L"OPTM_FEEDBACK_TEST")).c_str());
        snprintf(fbDetails_, sizeof(fbDetails_), "Test report from C:\\Users\\SomeOne\\Desktop\\x.exe and c:/users/other/a - please ignore.");
        fbLog_ = cmd.find("--feedback-long") != std::string::npos;
        if (fbLog_) for (int i = 0; i < 30; i++) strncat_s(fbDetails_, " A long report line that pushes the link past GitHub's limit.", _TRUNCATE);
        SendFeedback();
        Log("Feedback status: " + fbStatus_);
    }
    // first start of this version: show the welcome tour (not in screenshot test runs)
    if (!data_.tourDone && tourStep_ < 0 && cmd.find("--screenshot") == std::string::npos) StartTour();

    // developer check: --fps-self graphs this window's own frames
    if (cmd.find("--fps-self") != std::string::npos) {
        selfTest_ = true;
        frames_.ResetSession();
        if (frames_.Start({ GetCurrentProcessId() })) Log("FPS self-test: capturing this window's frames");
        else Log("FPS self-test failed: " + frames_.LastError());
    }

    lastUpdateCheck_ = Ms() - 6ull * 3600 * 1000 + 8000;   // first automatic check ~8s after start
    Tick();
}

// ------------------------------------------------------------ interface size
namespace {
const float kZooms[] = { 1.0f, 1.1f, 1.25f, 1.5f, 1.75f, 2.0f };
}

float App::Zoom() const {
    if (data_.uiScale > 0) return (float)data_.uiScale;
    // Automatic: at 100-ish% Windows scaling on a big screen, standard sizes look tiny - scale up
    MONITORINFO mi = { sizeof(mi) };
    HMONITOR mon = hwnd_ ? MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    if (!GetMonitorInfoW(mon, &mi)) return 1.0f;
    float logicalH = (mi.rcMonitor.bottom - mi.rcMonitor.top) / dpi_;   // height in "100%" pixels
    return logicalH >= 2000 ? 1.5f : logicalH >= 1300 ? 1.25f : 1.0f;
}

void App::SetZoom(double z) {
    data_.uiScale = z;
    data_.SaveConfig();
    rescale_ = true;
}

void App::OnDpiChanged(float dpi) {
    s_ *= dpi / dpi_;   // Windows resizes the window itself for this part
    dpi_ = dpi;
    rescale_ = true;
}

void App::Rescale() {
    float old = s_;
    s_ = dpi_ * Zoom();
    rescale_ = false;
    LoadFonts();
    ApplyTheme();
    // grow/shrink the window with the interface (unless maximized)
    if (old > 0 && std::fabs(old - s_) > 0.01f && !IsZoomed(hwnd_) && !IsIconic(hwnd_)) {
        RECT r; GetWindowRect(hwnd_, &r);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
        int w = std::min((int)((r.right - r.left) * s_ / old), (int)(mi.rcWork.right - mi.rcWork.left));
        int h = std::min((int)((r.bottom - r.top) * s_ / old), (int)(mi.rcWork.bottom - mi.rcWork.top));
        int x = std::clamp((int)(r.left - (w - (r.right - r.left)) / 2), (int)mi.rcWork.left, (int)mi.rcWork.right - w);
        int y = std::clamp((int)(r.top - (h - (r.bottom - r.top)) / 2), (int)mi.rcWork.top, (int)mi.rcWork.bottom - h);
        SetWindowPos(hwnd_, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

SIZE App::WindowSize(const RECT& work) const {
    SIZE s;
    s.cx = std::min((int)(1240 * s_), (int)(work.right - work.left - 40));
    s.cy = std::min((int)(820 * s_), (int)(work.bottom - work.top - 40));
    return s;
}

void App::LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    auto exists = [](const char* p) { return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES; };
    const char* regular = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* bold = "C:\\Windows\\Fonts\\seguisb.ttf";
    const char* heavy = "C:\\Windows\\Fonts\\segoeuib.ttf";
    const char* icons = exists("C:\\Windows\\Fonts\\SegoeIcons.ttf") ? "C:\\Windows\\Fonts\\SegoeIcons.ttf" : "C:\\Windows\\Fonts\\segmdl2.ttf";
    static const ImWchar iconRange[] = { 0xE700, 0xF8FF, 0 };

    auto add = [&](const char* path, float px) -> ImFont* {
        ImFontConfig cfg; cfg.OversampleH = 2;
        if (exists(path)) return io.Fonts->AddFontFromFileTTF(path, px * s_, &cfg);
        cfg.SizePixels = px * s_;                       // fallback keeps the intended size
        return io.Fonts->AddFontDefault(&cfg);
    };
    fontRegular_ = add(regular, 15);
    fontBold_    = add(exists(bold) ? bold : heavy, 15);
    fontTitle_   = add(heavy, 24);
    fontBig_     = add(heavy, 40);
    fontIcons_   = exists(icons) ? io.Fonts->AddFontFromFileTTF(icons, 17 * s_, nullptr, iconRange) : fontRegular_;
    io.FontDefault = fontRegular_;
}

void App::ApplyTheme() {
    Theme& t = data_.theme;
    const Palette* pal = nullptr;
    for (auto& p : kBackgrounds) if (t.background == p.name) pal = &p;
    if (!pal) { pal = &kBackgrounds[0]; t.background = pal->name; }
    const Corner* cor = nullptr;
    for (auto& c : kCorners) if (t.corners == c.name) cor = &c;
    if (!cor) { cor = &kCorners[0]; t.corners = cor->name; }
    if (t.accent.size() != 7 || t.accent[0] != '#' || t.accent.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos) t.accent = "#4F8BFF";
    g_accent = Hex(t.accent);
    g_card = Hex(pal->card); g_card2 = Hex(pal->card2); g_btn = Hex(pal->btn); g_chip = Hex(pal->chip); g_line = Hex(pal->line);
    g_cardR = cor->card * s_; g_btnR = cor->btn * s_;

    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();
    st.WindowPadding = ImVec2(0, 0);
    st.WindowRounding = 0; st.WindowBorderSize = 0;
    st.ChildRounding = g_cardR; st.ChildBorderSize = 0;
    st.FrameRounding = g_btnR; st.PopupRounding = g_btnR; st.GrabRounding = g_btnR;
    st.FramePadding = ImVec2(14 * s_, 7 * s_);
    st.ItemSpacing = ImVec2(8 * s_, 8 * s_);
    st.ScrollbarSize = 10 * s_; st.ScrollbarRounding = 5 * s_;
    st.PopupBorderSize = 1;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = Hex(pal->bg);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = g_card;
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.55f);
    c[ImGuiCol_Text] = kText;
    c[ImGuiCol_TextDisabled] = kDim;
    c[ImGuiCol_Border] = g_line;
    c[ImGuiCol_Separator] = g_line;
    c[ImGuiCol_Button] = g_btn;
    c[ImGuiCol_ButtonHovered] = Lighten(g_btn, 0.08f);
    c[ImGuiCol_ButtonActive] = Lighten(g_btn, 0.16f);
    c[ImGuiCol_FrameBg] = g_card2;
    c[ImGuiCol_FrameBgHovered] = g_card2;
    c[ImGuiCol_FrameBgActive] = g_card2;
    c[ImGuiCol_Header] = g_chip;
    c[ImGuiCol_HeaderHovered] = Lighten(g_chip, 0.05f);
    c[ImGuiCol_HeaderActive] = Lighten(g_chip, 0.1f);
    c[ImGuiCol_TableHeaderBg] = g_card2;
    c[ImGuiCol_TableBorderLight] = g_line;
    c[ImGuiCol_TableBorderStrong] = g_line;
    c[ImGuiCol_TableRowBgAlt] = Alpha(g_card2, 0.6f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = g_line;
    c[ImGuiCol_ScrollbarGrabHovered] = Lighten(g_line, 0.1f);
    c[ImGuiCol_ScrollbarGrabActive] = Lighten(g_line, 0.2f);
    c[ImGuiCol_TextSelectedBg] = Hex(t.accent, 0.35f);
    c[ImGuiCol_NavCursor] = g_accent;
}

void App::Log(const std::string& line) {
    log_.push_back("[" + util::NowStamp("%H:%M:%S") + "] " + line);
    static std::wstring logFile = util::EnvVar(L"OPTM_LOG_FILE");   // testing: mirror the log to a file
    if (!logFile.empty()) util::AppendFile(logFile, log_.back() + "\r\n");
    if (log_.size() > 2000) log_.erase(log_.begin(), log_.begin() + 500);
}

bool App::Busy() const { return selfTest_ || animating_ || ImGui::IsAnyItemActive(); }

// ============================================================ background
void App::Update() {
    for (auto& l : updater_.TakeLog()) Log(l);
    if (updater_.GetState() == Updater::Ready && !restart_) {
        restart_ = true;
        restartHidden_ = !IsWindowVisible(hwnd_);   // come back in the tray if that's where we were
        DestroyWindow(hwnd_);   // main.cpp restarts us after everything is restored
        return;
    }
    frames_.Pump();
    UpdateOverlay();
    uint64_t now = Ms();
    // A new build was put in place of our exe (Build.bat): restart into it once no game is running
    if (now - lastSelfCheck_ >= 5000) {
        lastSelfCheck_ = now;
        int64_t t = util::FileTime(selfPath_);
        if (selfStamp_ == 0) selfStamp_ = t;
        else if (t != 0 && t != selfStamp_ && !opt_.Active() && !restart_ && updater_.GetState() != Updater::Downloading) {
            Log("A new build of Project OptM was installed - restarting into it");
            restart_ = true;
            restartHidden_ = !IsWindowVisible(hwnd_);   // come back in the tray if that's where we were
            DestroyWindow(hwnd_);   // main.cpp puts everything back, then starts the new exe
            return;
        }
    }
    if (testZoom_ > 0 && now - startMs_ > 1000) { SetZoom(testZoom_); testZoom_ = 0; }
    if (now - lastTick_ >= (uint64_t)data_.settings.poll * 1000) Tick();
    if (data_.autoUpdate && updater_.Enabled() && !opt_.Active() && now - lastUpdateCheck_ >= 6ull * 3600 * 1000) {
        lastUpdateCheck_ = now;
        updater_.Check(false);
    }
}

void App::Tick() {
    lastTick_ = Ms();
    if (data_.ProfilesChanged()) { ReloadProfiles(); return; }
    procs_.Refresh();
    if (data_.autoDetect && data_.autoOptimize)
        for (auto& g : detector_.Scan(procs_, data_)) AddDetectedGame(g);
    std::set<std::string> now;
    for (auto& p : data_.profiles) if (opt_.Running(p, procs_)) now.insert(p.name);
    runningGames_ = now;
    opt_.Tick(procs_);
    if (const GameProfile* a = opt_.Active()) {
        std::vector<DWORD> pids;
        for (auto& e : a->exes) for (DWORD pid : procs_.Find(e)) pids.push_back(pid);
        if (selfTest_) pids.push_back(GetCurrentProcessId());
        if (frames_.Running()) frames_.SetPids(pids);
    }
    UpdateTray();
}

void App::ReloadProfiles() {
    std::string name = opt_.Active() ? opt_.Active()->name : "";
    if (!name.empty()) opt_.EndSession();
    data_.LoadProfiles();
    opt_.SyncLaunchPriority();
    RefreshChecks();
    if (hotkey_ && !data_.settings.panicHotkey) { UnregisterHotKey(hwnd_, kHotkeyPanic); hotkey_ = false; }
    if (!hotkey_ && data_.settings.panicHotkey) hotkey_ = RegisterHotKey(hwnd_, kHotkeyPanic, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_END) != 0;
    Log("Profiles reloaded - " + Plural(data_.profiles.size(), "game"));
    for (auto& w : data_.warnings) Log("  ! " + w);
    lastTick_ = 0;   // look for games again right away
}

void App::AddDetectedGame(const DetectedGame& g) {
    std::string name = data_.AppendProfile(g.name, g.exe, g.source, g.keep, g.antiCheat, g.antiCheatName);
    data_.autoAdded.push_back(name);
    data_.SaveConfig();
    data_.LoadProfiles();   // also records the new file time, so this isn't treated as a manual edit
    Log(">> New game found: " + name + " (" + g.source + ", " + g.exe + ".exe)" +
        (g.antiCheat ? " - " + g.antiCheatName + " anti-cheat, so it's in safe mode (launch priority applies from its next launch)" : "") +
        " - added to your games");
    if (g.antiCheat) opt_.SyncLaunchPriority();
    Balloon((name + " was added to your games and is being optimized." + (g.antiCheat ? " (anti-cheat safe mode)" : "")).c_str());
}

void App::NotAGame(const std::string& name) {
    const GameProfile* prof = nullptr;
    for (auto& p : data_.profiles) if (p.name == name) prof = &p;
    if (!prof) return;
    if (opt_.Active() && opt_.Active()->name == name) opt_.EndSession();
    for (auto& e : prof->exes) if (!util::Contains(data_.ignoredExes, e)) data_.ignoredExes.push_back(e);
    data_.RemoveProfile(name);
    data_.autoAdded.erase(std::remove(data_.autoAdded.begin(), data_.autoAdded.end(), name), data_.autoAdded.end());
    data_.SaveConfig();
    data_.LoadProfiles();
    opt_.SyncLaunchPriority();
    Log("Removed " + name + " - it won't be detected again (Settings > Reload after editing IgnoredExes in settings.json to undo)");
}

void App::RefreshChecks() {
    checks_ = RunChecks(sys_, data_.settings);
    lastChecks_ = Ms();
}

void App::SessionStarted(const GameProfile& p) {
    lastCompare_.clear();
    SYSTEM_POWER_STATUS ps;
    if (sys_.hasBattery && GetSystemPowerStatus(&ps) && ps.ACLineStatus == 0) {
        Log("  ! Running on battery - plug in the charger for full performance");
        Balloon((p.name + " is running on battery. Plug in the charger for full performance.").c_str(), NIIF_WARNING);
    }
    frames_.ResetSession();
    if (!data_.fpsOn) return;
    std::vector<DWORD> pids;
    for (auto& e : p.exes) for (DWORD pid : procs_.Find(e)) pids.push_back(pid);
    if (selfTest_) pids.push_back(GetCurrentProcessId());
    if (frames_.Start(pids)) Log("  FPS capture started");
    else if (frames_.Blocked()) Log("  No FPS for " + p.name + ": Windows refused frame capture (usually the game's anti-cheat) - optimizing still works");
    else Log("  FPS capture couldn't start: " + frames_.LastError());
}

void App::SessionEnded(const GameProfile& p, double minutes, bool gameClosed) {
    frames_.Stop();
    double avg = 0, low = 0;
    bool hasFps = frames_.SessionStats(avg, low);
    std::string extra;
    if (hasFps) extra += ", avg " + std::to_string((int)std::lround(avg)) + " FPS, 1% low " + std::to_string((int)std::lround(low));
    lastCompare_.clear();
    if (minutes >= 1) {
        double mins = std::round(minutes * 10) / 10;
        if (hasFps && mins >= 2) {
            for (auto it = data_.history.rbegin(); it != data_.history.rend(); ++it)
                if (it->game == p.name && it->avgFps > 0 && it->minutes >= 2) {
                    lastCompare_ = Signed(avg - it->avgFps) + " avg, " + Signed(low - it->low1) + " low vs last session";
                    extra += " (" + lastCompare_ + ")";
                    break;
                }
        }
        Session s;
        s.date = util::NowStamp("%Y-%m-%d %H:%M");
        s.game = p.name;
        s.minutes = mins;
        if (hasFps) { s.avgFps = std::round(avg * 10) / 10; s.low1 = std::round(low * 10) / 10; }
        data_.AddSession(s, OPTM_VERSION);
    }
    if (opt_.SessionCleanups() > 0) extra += ", RAM cleared " + std::to_string(opt_.SessionCleanups()) + "x";
    Log("<< " + p.name + (gameClosed ? " closed after " : " session ended after ") + util::FormatDuration(minutes) + extra + " - everything restored");
}

void App::Shutdown() {
    if (shutdown_) return;
    shutdown_ = true;
    if (hotkey_) UnregisterHotKey(hwnd_, kHotkeyPanic);
    if (overlayHotkey_) UnregisterHotKey(hwnd_, kHotkeyOverlay);
    overlay_.Destroy();
    sensors_.Stop();
    opt_.EndSession();
    // fully closing (not restarting into a new build): the per-game Windows settings go back too
    if (data_.revertOnExit && !restart_) opt_.RevertOnExit();
    frames_.Stop();
    if (trayAdded_) Shell_NotifyIconW(NIM_DELETE, &nid_);
    if (nid_.hIcon) DestroyIcon(nid_.hIcon);
}

// ============================================================ tray + window events
void App::UpdateTray() {
    std::string tip;
    if (!data_.autoOptimize) tip = "Project OptM - paused";
    else if (opt_.Active()) tip = "Optimizing: " + opt_.Active()->name;
    else tip = "Project OptM - watching";
    if (tip == trayTip_ || !trayAdded_) return;
    trayTip_ = tip;
    std::wstring w = util::Widen(tip).substr(0, 127);
    wcscpy_s(nid_.szTip, w.c_str());
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void App::Balloon(const char* text, DWORD icon) {
    if (!trayAdded_) return;
    NOTIFYICONDATAW n = nid_;
    n.uFlags = NIF_INFO;
    n.dwInfoFlags = icon;
    wcscpy_s(n.szInfoTitle, L"Project OptM");
    wcscpy_s(n.szInfo, util::Widen(text).substr(0, 255).c_str());
    Shell_NotifyIconW(NIM_MODIFY, &n);
}

void App::OnTaskbarCreated() {   // Explorer restarted: put the icon back
    trayTip_.clear();
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayAdded_ = Shell_NotifyIconW(NIM_ADD, &nid_) != 0;
    UpdateTray();
}

void App::OnTray(LPARAM lp) {
    if (lp == WM_LBUTTONDBLCLK || lp == WM_LBUTTONUP) { ShowMain(); return; }
    if (lp != WM_RBUTTONUP && lp != WM_CONTEXTMENU) return;
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, 1, L"Open");
    AppendMenuW(m, MF_STRING, 2, L"Panic: undo everything now");
    AppendMenuW(m, MF_STRING | (data_.autoOptimize ? MF_CHECKED : 0), 3, L"Auto-optimize");
    AppendMenuW(m, MF_STRING, 5, L"Send feedback...");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, 4, L"Exit");
    POINT pt; GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    UINT cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd_, nullptr);
    PostMessageW(hwnd_, WM_NULL, 0, 0);
    DestroyMenu(m);
    if (cmd) OnCommand(cmd);
}

void App::OnCommand(WPARAM id) {
    switch (id) {
        case 1: ShowMain(); break;
        case 2: Panic(); break;
        case 3: ToggleAuto(); break;
        case 4: DestroyWindow(hwnd_); break;
        case 5: ShowMain(); OpenFeedback(); break;
    }
}

void App::OnHotkey() { Panic(); }

void App::ToggleOverlay() {
    data_.overlayOn = !data_.overlayOn;
    data_.SaveConfig();
    Log(data_.overlayOn ? "In-game overlay on (Ctrl+Alt+O hides it)" : "In-game overlay off (Ctrl+Alt+O shows it)");
}

// The game's main window: its biggest visible top-level window
HWND App::GameWindow() {
    uint64_t now = Ms();
    if (gameWnd_ && IsWindow(gameWnd_) && now - lastGameWnd_ < 2000) return gameWnd_;
    lastGameWnd_ = now;
    gameWnd_ = nullptr;
    const GameProfile* g = opt_.Active();
    if (!g) return nullptr;
    struct Find { std::set<DWORD> pids; HWND best = nullptr; long long area = 0; } f;
    for (auto& e : g->exes) for (DWORD pid : procs_.Find(e)) f.pids.insert(pid);
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* f = (Find*)lp;
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        if (!f->pids.count(pid) || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
        RECT r; GetWindowRect(h, &r);
        long long a = (long long)(r.right - r.left) * (r.bottom - r.top);
        if (a > f->area) { f->area = a; f->best = h; }
        return TRUE;
    }, (LPARAM)&f);
    gameWnd_ = f.best;
    return gameWnd_;
}

namespace {
// what the overlay can show under the FPS, in order (ids are kept in settings.json "Overlay" > "Items")
const std::pair<const char*, const char*> kOverlayItems[] = {
    { "low", "1% low" }, { "frametime", "Frametime" }, { "graph", "Frametime graph" },
    { "gpu", "GPU usage" }, { "gputemp", "GPU temp" }, { "vram", "VRAM" }, { "cpu", "CPU usage" }, { "ram", "RAM" },
};
std::wstring Num(double v, const wchar_t* fmt) { wchar_t b[48]; swprintf(b, 48, fmt, v); return b; }
}

void App::UpdateOverlay() {
    uint64_t now = Ms();
    if (now - lastOverlay_ < 250) return;   // 4 updates a second, like Afterburner's
    lastOverlay_ = now;
    HWND game = nullptr;
    bool want = false;
    if (overlayDemo_) { game = hwnd_; want = data_.overlayOn; }
    else if (const GameProfile* g = opt_.Active()) {
        want = data_.overlayOn && data_.fpsOn && !frames_.Blocked() && (!g->antiCheat || data_.overlayAntiCheat);
        if (want) {
            game = GameWindow();
            // only while you're in the game - alt-tab away and it goes
            DWORD fg = 0, gp = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &fg);
            if (game) GetWindowThreadProcessId(game, &gp);
            want = game && !IsIconic(game) && fg == gp;
        }
    }
    auto has = [&](const char* id) { return util::Contains(data_.overlayItems, id); };
    sensors_.SetActive((want || page_ == Overlay) && (has("gpu") || has("gputemp") || has("vram") || has("cpu") || has("ram")));
    if (!want) { overlay_.Hide(); return; }
    overlay_.Show(game, data_.overlayX, data_.overlayY, OverlayScale(), OverlayNow());
}

float App::OverlayScale() const { return Zoom() * data_.overlaySize / 100.0f; }

// What the overlay shows right now (also drawn small in the Overlay page's screen preview)
OverlayContent App::OverlayNow() {
    auto has = [&](const char* id) { return util::Contains(data_.overlayItems, id); };
    const auto& buf = frames_.Buffer();
    frames::Live st = frames::Stats(buf);
    Readings rd = sensors_.Get();
    OverlayContent c;
    c.hasFps = buf.size() >= 10;
    c.fps = st.fps;
    const std::wstring none = L"--";
    if (has("low")) c.stats.push_back({ L"1% low", c.hasFps && st.low1 > 0 ? Num(st.low1, L"%.0f") : none });
    if (has("frametime")) c.stats.push_back({ L"Frame", c.hasFps ? Num(st.frametime, L"%.1f ms") : none });
    if (has("gpu") || has("gputemp")) {
        std::wstring v;
        if (has("gpu")) v = rd.gpuPct >= 0 ? Num(rd.gpuPct, L"%.0f%%") : none;
        if (has("gputemp") && rd.gpuTempC >= 0) v += (v.empty() ? L"" : L"  ") + Num(rd.gpuTempC, L"%.0f°C");
        else if (has("gputemp") && v.empty()) v = none;
        c.stats.push_back({ L"GPU", v });
    }
    if (has("vram")) c.stats.push_back({ L"VRAM", rd.vramUsedGB >= 0 ? Num(rd.vramUsedGB, L"%.1f") + Num(rd.vramTotalGB, L"/%.0f GB") : none });
    if (has("cpu")) c.stats.push_back({ L"CPU", rd.cpuPct >= 0 ? Num(rd.cpuPct, L"%.0f%%") : none });
    if (has("ram")) c.stats.push_back({ L"RAM", rd.ramUsedGB >= 0 ? Num(rd.ramUsedGB, L"%.1f") + Num(rd.ramTotalGB, L"/%.0f GB") : none });
    c.showGraph = has("graph");
    if (c.showGraph) c.graph = frames::Columns(buf, 4000, 64);
    c.accent = RGB((int)(g_accent.x * 255), (int)(g_accent.y * 255), (int)(g_accent.z * 255));
    c.opacity = data_.overlayOpacity;
    return c;
}

void App::OnActivate() { if (Ms() - lastChecks_ >= 10000) RefreshChecks(); }

void App::OnMinimize() {
    ShowWindow(hwnd_, SW_HIDE);
    if (!trayTipShown_) { Balloon("Still running here. Double-click the icon to open."); trayTipShown_ = true; }
}

void App::ShowMain() {
    ShowWindow(hwnd_, SW_SHOW);
    ShowWindow(hwnd_, SW_RESTORE);
    SetForegroundWindow(hwnd_);
}

// ============================================================ actions
void App::ToggleAuto() {
    opt_.SetAuto(!data_.autoOptimize);
    Tick();
}

void App::Panic() {
    opt_.Panic();
    UpdateTray();
    Balloon("Panic: everything restored. Auto-optimize is paused.", NIIF_WARNING);
}

bool App::PickLauncher(const std::string& game) {
    wchar_t file[MAX_PATH * 2] = {};
    std::wstring title = L"Pick the shortcut or .exe for " + util::Widen(game);
    wchar_t desktop[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop);
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Games and shortcuts (*.exe;*.lnk;*.url)\0*.exe;*.lnk;*.url\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH * 2;
    ofn.lpstrTitle = title.c_str();
    ofn.lpstrInitialDir = desktop;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return false;
    std::string path = util::Narrow(file);
    data_.launchPaths[game] = path;
    data_.SaveConfig();
    Log(game + " will launch: " + path.substr(path.find_last_of("\\/") + 1));
    return true;
}

void App::StartGame(const std::string& game) {
    auto it = data_.launchPaths.find(game);
    if (it == data_.launchPaths.end() || GetFileAttributesW(util::Widen(it->second).c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!PickLauncher(game)) return;
        it = data_.launchPaths.find(game);
    }
    Log("Launching " + game + "...");
    util::OpenAsUser(util::Widen(it->second));   // runs as you, not as admin
}

void App::ShowPlan(const GameProfile& p) {
    std::string txt = "When " + p.name + " runs, Project OptM will:\n\n";
    for (auto& l : opt_.Plan(p)) txt += "  -  " + l + "\n";
    txt += "\nEverything is put back when the game closes.";
    Msg(hwnd_, txt, "Project OptM - " + p.name, MB_OK | MB_ICONINFORMATION);
}

void App::ClearShaderCache() {
    if (opt_.Active()) { Log("Close " + opt_.Active()->name + " first - its shader cache is in use"); return; }
    std::vector<std::string> makers;
    for (auto& g : sys_.gpus) if (g.vendor != "Other" && !util::Contains(makers, g.vendor)) makers.push_back(g.vendor);
    std::string q = "Clear the DirectX" + (makers.empty() ? std::string() : " and " + util::Join(makers, " / ")) +
                    " shader caches?\n\nThis fixes stutter or crashes after a driver update. The first launch of each game afterwards may stutter for a minute while shaders rebuild.";
    if (Msg(hwnd_, q, "Project OptM", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    std::wstring local = util::LocalAppDataRoot(), user = util::EnvVar(L"USERPROFILE"), pd = util::EnvVar(L"ProgramData");
    std::vector<std::wstring> dirs = { local + L"\\D3DSCache" };
    if (util::Contains(makers, "AMD"))    for (auto d : { L"\\AMD\\DxCache", L"\\AMD\\DxcCache", L"\\AMD\\VkCache", L"\\AMD\\GLCache" }) dirs.push_back(local + d);
    if (util::Contains(makers, "NVIDIA")) { dirs.push_back(local + L"\\NVIDIA\\DXCache"); dirs.push_back(local + L"\\NVIDIA\\GLCache"); dirs.push_back(pd + L"\\NVIDIA Corporation\\NV_Cache"); }
    if (util::Contains(makers, "Intel"))  { dirs.push_back(local + L"\\Intel\\ShaderCache"); dirs.push_back(user + L"\\AppData\\LocalLow\\Intel\\ShaderCache"); }
    SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    uint64_t freed = 0; int files = 0, skipped = 0;
    namespace fs = std::filesystem;
    for (auto& d : dirs) {
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        std::vector<fs::path> list;
        for (auto it = fs::recursive_directory_iterator(d, fs::directory_options::skip_permission_denied, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
            if (it->is_regular_file(ec)) list.push_back(it->path());
        for (auto& f : list) {
            uint64_t len = fs::file_size(f, ec);
            if (ec) len = 0;
            if (fs::remove(f, ec) && !ec) { freed += len; files++; } else skipped++;
        }
    }
    std::string msg = "Shader cache cleared: " + std::to_string(files) + " files, " + std::to_string(freed / (1024 * 1024)) + " MB freed";
    if (skipped) msg += " (" + std::to_string(skipped) + " in use, skipped)";
    Log(msg);
}

void App::OpenProfiles() {
    if (GetFileAttributesW(data_.ProfilesPath().c_str()) == INVALID_FILE_ATTRIBUTES) data_.LoadProfiles();   // recreates it
    ShellExecuteW(nullptr, L"open", L"notepad.exe", (L"\"" + data_.ProfilesPath() + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
}

// ============================================================ widgets
bool App::BeginCard(const char* id, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18 * s_, 14 * s_));
    ImGuiChildFlags f = ImGuiChildFlags_AlwaysUseWindowPadding | (height <= 0 ? ImGuiChildFlags_AutoResizeY : 0);
    bool open = ImGui::BeginChild(id, ImVec2(0, height), f, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    return open;
}
void App::EndCard() { ImGui::EndChild(); }

void App::Label(const char* text) {
    ImGui::PushFont(fontBold_);
    ImGui::PushStyleColor(ImGuiCol_Text, kDim);
    ImGui::SetWindowFontScale(0.8f);
    ImGui::TextUnformatted(text);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

bool App::AccentButton(const char* label, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, g_accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Lighten(g_accent, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Lighten(g_accent, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_Text, TextOn(g_accent));
    bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return r;
}

// The OptM mark: black tile, accent ring, white needle
void App::Logo(float size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float k = size / 40.f;
    float r = g_cardR * 0.8f;
    dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), IM_COL32(10, 12, 16, 255), r);
    dl->AddRect(p, ImVec2(p.x + size, p.y + size), U32(Hex(data_.theme.accent, 0.55f)), r, 0, 1.0f);
    ImVec2 c(p.x + 20 * k, p.y + 20 * k);
    dl->AddCircle(c, 11.5f * k, U32(g_accent), 48, 4.6f * k);
    dl->AddLine(c, ImVec2(p.x + 24.6f * k, p.y + 15.4f * k), IM_COL32_WHITE, 2.6f * k);
    dl->AddCircleFilled(c, 2.1f * k, IM_COL32_WHITE, 16);
    ImGui::Dummy(ImVec2(size, size));
}

void App::VendorBadge(const std::string& vendor) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushFont(fontBold_);
    ImVec2 ts = ImGui::CalcTextSize(vendor.c_str());
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 pad(6 * s_, 2 * s_);
    dl->AddRectFilled(p, ImVec2(p.x + ts.x + pad.x * 2, p.y + ts.y + pad.y * 2), U32(VendorColor(vendor)), 4 * s_);
    dl->AddText(ImVec2(p.x + pad.x, p.y + pad.y), IM_COL32_WHITE, vendor.c_str());
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(ts.x + pad.x * 2, ts.y + pad.y * 2));
}

// Health check chips, wrapped into rows. Outlined ones can be clicked to fix.
void App::Chips() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float maxX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    float gap = 6 * s_;
    ImVec2 pad(10 * s_, 5 * s_);
    float dot = 8 * s_;
    int fixIndex = -1;
    ImGui::SetWindowFontScale(0.87f);
    for (size_t i = 0; i < checks_.size(); i++) {
        const Check& c = checks_[i];
        ImVec2 ts = ImGui::CalcTextSize(c.text.c_str());
        ImVec2 size(pad.x * 2 + dot + 6 * s_ + ts.x, pad.y * 2 + ts.y);
        if (i > 0) {
            ImGui::SameLine(0, gap);
            if (ImGui::GetCursorScreenPos().x + size.x > maxX) ImGui::NewLine();
        }
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID((int)i);
        if (ImGui::InvisibleButton("chip", size) && c.fix) fixIndex = (int)i;
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        float r = std::min(g_btnR * 1.8f, size.y / 2);
        dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), U32(hov && c.fix ? Lighten(g_chip, 0.06f) : g_chip), r);
        if (c.fix) dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), U32(kSub), r, 0, 1.0f);
        ImVec4 dc = c.state == Check::Ok ? kGreen : c.state == Check::Warn ? kAmber : g_accent;
        dl->AddCircleFilled(ImVec2(p.x + pad.x + dot / 2, p.y + size.y / 2), dot / 2, U32(dc), 16);
        dl->AddText(ImVec2(p.x + pad.x + dot + 6 * s_, p.y + pad.y), U32(Alpha(kText, 0.9f)), c.text.c_str());
        if (hov) {
            ImGui::SetWindowFontScale(1.0f);
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(360 * s_);
            ImGui::TextUnformatted(c.tip.c_str());
            if (c.fix) { ImGui::Dummy(ImVec2(0, 2 * s_)); ImGui::TextColored(g_accent, "%s", c.fixLabel.c_str()); }
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::SetWindowFontScale(0.87f);
        }
    }
    ImGui::SetWindowFontScale(1.0f);
    if (fixIndex >= 0) {
        Check c = checks_[fixIndex];
        std::string extra = c.fix();
        Log("Fix applied: " + c.text);
        if (!extra.empty()) Log(extra);
        RefreshChecks();
    }
}

std::vector<std::pair<std::string, std::string>> App::HardwareLines() const {
    std::vector<std::pair<std::string, std::string>> l;
    std::string best = SystemInfo::MaskText(sys_.bestMask);
    uint64_t bg = opt_.BackgroundMask();
    std::string t;
    switch (sys_.layout) {
        case CpuLayout::DualX3D:
            t = "Games run on the V-Cache cores (CPU " + best + ")";
            if (bg) t += ", and background apps move to the other CCD (CPU " + SystemInfo::MaskText(bg) + ") while you play";
            l.push_back({ sys_.cpuVendor, t + ". Balanced power plan is kept so AMD's core parking works." });
            break;
        case CpuLayout::Hybrid:
            t = "Games run on the P-cores (CPU " + best + ")";
            if (bg) t += ", and background apps move to the E-cores (CPU " + SystemInfo::MaskText(bg) + ") while you play";
            l.push_back({ sys_.cpuVendor, t + "." });
            break;
        case CpuLayout::SingleX3D: l.push_back({ sys_.cpuVendor, "Every core has the extra 3D V-Cache, so no core pinning is needed." }); break;
        case CpuLayout::DualCCD:   l.push_back({ sys_.cpuVendor, "Two CCDs without V-Cache - Windows schedules them well on its own, so no pinning is needed." }); break;
        default:                   l.push_back({ sys_.cpuVendor, "Single core cluster - no core pinning needed." }); break;
    }
    if (sys_.IsRaptorLake()) l.push_back({ "Intel", "Microcode is checked for Intel's 13th/14th gen stability fixes (0x12B, and 0x12F as the latest)." });
    if (sys_.IsArrowLake()) l.push_back({ "Intel", "Core Ultra 200S: checked for Intel's gaming performance fixes (microcode 0x114+ and Windows 11 24H2)." });
    if (sys_.apo) l.push_back({ "Intel", "Intel APO is installed, so games are steered to the P-cores with CPU sets (soft pinning) instead of locked to them." });
    else if (sys_.layout == CpuLayout::Hybrid)
        l.push_back({ "Intel", "Games that use many threads can use cores = Prefer in profiles.ini (or Soft core pinning on the Tweaks page) so they can spill onto the E-cores." });
    if (sys_.hasBattery) l.push_back({ sys_.cpuVendor, "Laptop: you're warned when a game starts on battery, and the Tweaks page can switch Windows to Best performance mode while you play." });
    std::set<std::string> seen;
    bool apps = data_.settings.vendorApps;
    for (auto& g : sys_.gpus) {
        if (!seen.insert(g.vendor).second) continue;
        std::string list = util::Join(opt_.VendorApps(g.vendor), ", ");
        if (g.vendor == "AMD")
            l.push_back({ "AMD", apps ? "Radeon: Adrenalin helpers (" + list + ") get low priority while you play; driver age is checked." : "Radeon: driver age is checked." });
        else if (g.vendor == "NVIDIA")
            l.push_back({ "NVIDIA", apps ? "GeForce: NVIDIA app and overlay get low priority while you play; driver age and GPU scheduling (for DLSS Frame Generation) are checked." : "GeForce: driver age is checked." });
        else if (g.vendor == "Intel")
            l.push_back({ "Intel", apps ? "Intel graphics: Intel Graphics Software gets low priority while you play; driver age is checked." : "Intel graphics: driver age is checked." });
    }
    if (sys_.gpus.size() >= 2 && data_.settings.forceGpu) l.push_back({ sys_.gpus[0].vendor, "Games are locked to " + sys_.gpus[0].name + ", not the integrated graphics." });
    return l;
}

// ============================================================ frame
void App::Render() {
    ImGuiIO& io = ImGui::GetIO();
    animating_ = animNext_;   // something was still moving last frame: main.cpp keeps drawing
    animNext_ = false;
    marks_.clear();

    // Ctrl+1..8 switches tabs; Ctrl + / - / 0 changes the interface size
    if (io.KeyCtrl && !io.WantTextInput) {
        for (int i = 0; i < PageCount; i++)
            if (ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_1 + i), false) || ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_Keypad1 + i), false)) page_ = (Page)i;
        float z = Zoom();
        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false)) {
            for (float k : kZooms) if (k > z + 0.01f) { SetZoom(k); break; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) {
            for (int i = (int)(sizeof(kZooms) / sizeof(kZooms[0])) - 1; i >= 0; i--) if (kZooms[i] < z - 0.01f) { SetZoom(kZooms[i]); break; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_0, false) || ImGui::IsKeyPressed(ImGuiKey_Keypad0, false)) SetZoom(0);
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false)) {   // Ctrl+F: search games on the Games page, tweaks anywhere else
        if (page_ == Games) focusGameSearch_ = true;
        else { page_ = Tweaks; focusSearch_ = true; }
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    float pad = 16 * s_;
    float h = io.DisplaySize.y - pad * 2;
    ImGui::SetCursorPos(ImVec2(pad, pad));
    Sidebar(h);
    ImGui::SameLine(0, pad);

    ImGui::BeginChild("content", ImVec2(io.DisplaySize.x - ImGui::GetCursorPosX() - pad, h), 0, ImGuiWindowFlags_NoScrollbar);
    StatusBar();
    ImGui::Dummy(ImVec2(0, 4 * s_));
    static const char* titles[] = { "Home", "Games", "Sessions", "Overlay", "Tweaks", "System", "Activity", "Settings" };
    ImGui::PushFont(fontTitle_);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4 * s_);
    ImGui::TextUnformatted(titles[page_]);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 2 * s_));

    // page transition: the new page fades in and settles upward a few pixels
    if (page_ != lastPage_) { lastPage_ = page_; anim_["page"] = data_.animations ? 0.0f : 1.0f; }
    float t = Anim("page", 1.0f, 11.0f);
    float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0f - ease) * 10 * s_);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.15f + 0.85f * ease);
    ImGui::BeginChild("page", ImVec2(0, 0), 0, page_ == Activity ? ImGuiWindowFlags_NoScrollbar : 0);
    if (testScroll_ > 0 && ImGui::GetFrameCount() > 3) ImGui::SetScrollY(testScroll_ * s_);
    {
        ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
        marks_["page"] = ImVec4(wp.x, wp.y, wp.x + ws.x, wp.y + ws.y);
    }
    switch (page_) {
        case Home: PageHome(); break;
        case Games: PageGames(); break;
        case Sessions: PageSessions(); break;
        case Overlay: PageOverlay(); break;
        case System: PageSystem(); break;
        case Activity: PageActivity(); break;
        case Settings: PageSettings(); break;
        case Tweaks: PageTweaks(); break;
        default: break;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    HistoryPopup();
    GameSettingsPopup();
    FeedbackPopup();
    ImGui::End();
    TourOverlay();
}

// ------------------------------------------------------------ animation + tour
float App::Anim(const std::string& key, float target, float speed, float init) {
    auto it = anim_.find(key);
    if (it == anim_.end()) it = anim_.emplace(key, init < 0 ? target : init).first;
    float& v = it->second;
    if (!data_.animations) return v = target;
    float dt = std::min(ImGui::GetIO().DeltaTime, 1.0f / 30.0f);   // after an idle pause, don't jump
    v += (target - v) * std::min(1.0f, dt * speed);
    if (std::fabs(target - v) < 0.002f * std::max(1.0f, std::fabs(target))) v = target;
    else animNext_ = true;
    return v;
}

void App::Mark(const char* key) {
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    marks_[key] = ImVec4(a.x, a.y, b.x, b.y);
}

namespace {
struct TourStep { int page; const char* mark; const char* title; const char* text; };
// page numbers follow App::Page: Home 0, Games 1, Sessions 2, Overlay 3, Tweaks 4, System 5, Activity 6, Settings 7
const TourStep kTour[] = {
    { 0, "", "Welcome to Project OptM",
      "Project OptM spots the game you're playing and tunes Windows for it - the best CPU cores, priorities, background apps "
      "and more - then puts everything back when you close the game.\n\nThis quick tour shows you around. It takes a minute." },
    { 0, "status", "What's happening right now",
      "This bar shows the game being optimized and what was done for it. AUTO-OPTIMIZE turns everything on or off." },
    { 0, "nav", "Pages",
      "Home, Games, Sessions, Overlay, Tweaks, System, Activity and Settings. Ctrl + 1 to 8 jumps straight to one." },
    { 0, "perf", "Live performance",
      "While a game runs you see its FPS, 1% lows and a frametime graph here. Every session is saved with its playtime and FPS "
      "on the Sessions page, so you can see how a game improves - and the Overlay page puts the numbers over the game itself." },
    { 1, "games", "Your games",
      "Every game with a profile, A to Z. Games you haven't set up are added automatically the first time they start - look "
      "for the NEW tag.\n\nCtrl + F searches, PLAY launches a game, and the (i) button shows exactly what gets changed for it." },
    { 4, "presets", "Tweaks",
      "Pick Safe, Balanced or Aggressive, or make your own preset. Click any tweak to read what it does, its pros and its cons. "
      "Tweaks are applied when a game starts and undone when it closes." },
    { 5, "checks", "Health checks",
      "Checks your RAM speed, refresh rate, drivers, Game Mode and more for your exact hardware. Outlined checks can be "
      "fixed with one click." },
    { 0, "footer", "Undo everything, any time",
      "Project OptM keeps running in the tray. Ctrl + Alt + End - or Panic in the tray menu - instantly undoes every change.\n\n"
      "You can replay this tour from Settings whenever you like." },
};
const int kTourSteps = (int)(sizeof(kTour) / sizeof(kTour[0]));
}  // namespace

void App::StartTour() { TourGo(0); }

void App::TourGo(int step) {
    if (step < 0) step = 0;
    if (step >= kTourSteps) {   // finished (or skipped)
        tourStep_ = -1;
        data_.tourDone = true;
        data_.SaveConfig();
        page_ = Home;
        return;
    }
    tourStep_ = step;
    page_ = (Page)kTour[step].page;
    anim_["tour.card"] = data_.animations ? 0.0f : 1.0f;
}

void App::TourOverlay() {
    if (tourStep_ < 0) return;
    const TourStep& st = kTour[tourStep_];
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 disp = io.DisplaySize;

    // spotlight: the target's rect (padded), gliding between steps; none = centered card on a dimmed screen
    auto mk = marks_.find(st.mark);
    bool has = *st.mark && mk != marks_.end();
    float pad = 8 * s_;
    ImVec4 r = has ? ImVec4(mk->second.x - pad, mk->second.y - pad, mk->second.z + pad, mk->second.w + pad)
                   : ImVec4(disp.x / 2, disp.y / 2, disp.x / 2, disp.y / 2);
    r.x = std::max(r.x, 4 * s_); r.y = std::max(r.y, 4 * s_);
    r.z = std::min(r.z, disp.x - 4 * s_); r.w = std::min(r.w, disp.y - 4 * s_);
    float x0 = Anim("tour.x0", r.x, 12, disp.x / 2), y0 = Anim("tour.y0", r.y, 12, disp.y / 2);
    float x1 = Anim("tour.x1", r.z, 12, disp.x / 2), y1 = Anim("tour.y1", r.w, 12, disp.y / 2);
    float fade = Anim("tour.dim", 1.0f, 10, 0.0f);
    float cardA = Anim("tour.card", 1.0f, 12, 0.0f);

    // a full-screen layer that dims everything and swallows clicks outside the card
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(disp);
    ImGui::Begin("##tourlayer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::InvisibleButton("##block", disp);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 dim = U32(ImVec4(0, 0, 0, 0.66f * fade));
    if (x1 - x0 > 2 && y1 - y0 > 2) {
        dl->AddRectFilled(ImVec2(0, 0), ImVec2(disp.x, y0), dim);
        dl->AddRectFilled(ImVec2(0, y1), disp, dim);
        dl->AddRectFilled(ImVec2(0, y0), ImVec2(x0, y1), dim);
        dl->AddRectFilled(ImVec2(x1, y0), ImVec2(disp.x, y1), dim);
        dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), U32(Alpha(g_accent, fade)), g_cardR + pad * 0.5f, 0, 2.0f * s_);
    } else {
        dl->AddRectFilled(ImVec2(0, 0), disp, dim);
    }
    ImGui::End();

    // the card: below the spotlight if it fits, else above, else beside; centered when there's no target
    float cardW = 400 * s_, cardH = 230 * s_, gap = 16 * s_;
    ImVec2 pos;
    if (!has) pos = ImVec2((disp.x - cardW) / 2, (disp.y - cardH) / 2);
    else if (r.w + gap + cardH < disp.y) pos = ImVec2(r.x, r.w + gap);
    else if (r.y - gap - cardH > 0) pos = ImVec2(r.x, r.y - gap - cardH);
    else if (r.z + gap + cardW < disp.x) pos = ImVec2(r.z + gap, r.y);
    else pos = ImVec2(r.x - gap - cardW, r.y);
    pos.x = std::clamp(pos.x, 12 * s_, disp.x - cardW - 12 * s_);
    pos.y = std::clamp(pos.y, 12 * s_, std::max(12 * s_, disp.y - cardH - 12 * s_)) + (1.0f - cardA) * 8 * s_;
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(ImVec2(cardW, 0));
    ImGui::SetNextWindowFocus();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, cardA);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22 * s_, 18 * s_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, g_cardR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Lighten(g_card, 0.03f));
    ImGui::PushStyleColor(ImGuiCol_Border, Alpha(g_accent, 0.6f));
    ImGui::Begin("##tourcard", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_AlwaysAutoResize);
    char b[48];
    snprintf(b, sizeof(b), "TOUR  %d / %d", tourStep_ + 1, kTourSteps);
    Label(b);
    ImGui::PushFont(fontTitle_);
    ImGui::SetWindowFontScale(0.8f);
    ImGui::TextUnformatted(st.title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 2 * s_));
    ImGui::PushTextWrapPos(cardW - 44 * s_);
    ImGui::TextColored(kSub, "%s", st.text);
    ImGui::PopTextWrapPos();
    ImGui::Dummy(ImVec2(0, 8 * s_));

    // progress dots
    ImDrawList* cd = ImGui::GetWindowDrawList();
    ImVec2 dp = ImGui::GetCursorScreenPos();
    for (int i = 0; i < kTourSteps; i++)
        cd->AddCircleFilled(ImVec2(dp.x + 4 * s_ + i * 14 * s_, dp.y + 4 * s_), (i == tourStep_ ? 4.0f : 3.0f) * s_,
                            U32(i == tourStep_ ? g_accent : Alpha(kDim, 0.6f)), 12);
    ImGui::Dummy(ImVec2(kTourSteps * 14 * s_, 8 * s_));
    ImGui::Dummy(ImVec2(0, 6 * s_));

    bool last = tourStep_ == kTourSteps - 1;
    int go = tourStep_;
    if (!last) { if (ImGui::Button("Skip tour")) go = kTourSteps; ImGui::SameLine(); }
    const char* next = last ? "Finish" : tourStep_ == 0 ? "Show me around" : "Next";
    float nextW = ImGui::CalcTextSize(next).x + ImGui::GetStyle().FramePadding.x * 2;
    float backW = ImGui::CalcTextSize("Back").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - nextW - (tourStep_ > 0 ? backW + 8 * s_ : 0));
    if (tourStep_ > 0) { if (ImGui::Button("Back")) go = tourStep_ - 1; ImGui::SameLine(0, 8 * s_); }
    if (AccentButton(next, ImVec2(0, 0))) go = tourStep_ + 1;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) go = kTourSteps;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)) go = tourStep_ + 1;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false) && tourStep_ > 0) go = tourStep_ - 1;
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
    if (go != tourStep_) TourGo(go);
}

void App::Sidebar(float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14 * s_, 16 * s_));
    ImGui::BeginChild("sidebar", ImVec2(220 * s_, height), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // logo + name
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4 * s_);
    Logo(38 * s_);
    ImGui::SameLine(0, 12 * s_);
    ImGui::BeginGroup();
    ImGui::PushFont(fontTitle_);
    ImGui::SetWindowFontScale(0.72f);
    ImGui::TextUnformatted("PROJECT");
    ImGui::SameLine(0, 5 * s_);
    ImGui::TextColored(g_accent, "OPTM");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::TextColored(kDim, "v" OPTM_VERSION);
    ImGui::EndGroup();
    ImGui::Dummy(ImVec2(0, 14 * s_));

    // navigation
    static const char* names[] = { "Home", "Games", "Sessions", "Overlay", "Tweaks", "System", "Activity", "Settings" };
    static const unsigned icons[] = { 0xE80F, 0xE7FC, 0xE823, 0xE890, 0xE9E9, 0xE7F4, 0xE81C, 0xE713 };
    ImDrawList* dl = ImGui::GetWindowDrawList();
    int warns = (int)std::count_if(checks_.begin(), checks_.end(), [](const Check& c) { return c.state == Check::Warn; });
    // highlights go on a layer under the icons and labels, so the selection can glide between tabs
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    ImVec2 navTop = ImGui::GetCursorScreenPos();
    float navW = ImGui::GetContentRegionAvail().x, hh = 42 * s_, selY = 0, navBottom = navTop.y;
    for (int i = 0; i < PageCount; i++) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = navW;
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("nav", ImVec2(w, hh))) page_ = (Page)i;
        bool hov = ImGui::IsItemHovered(), sel = page_ == i;
        ImGui::PopID();
        if (sel) selY = p.y - navTop.y;
        float hv = Anim("nav.hover" + std::to_string(i), hov && !sel ? 1.0f : 0.0f, 18.0f);
        if (hv > 0.01f) {
            dl->ChannelsSetCurrent(0);
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + hh), U32(Alpha(g_chip, 0.6f * hv)), g_btnR);
            dl->ChannelsSetCurrent(1);
        }
        ImU32 col = U32(sel ? kText : kSub);
        std::string ic = Icon(icons[i]);
        dl->AddText(fontIcons_, fontIcons_->FontSize, ImVec2(p.x + 22 * s_, p.y + (hh - fontIcons_->FontSize) / 2), col, ic.c_str());
        dl->AddText(fontBold_, fontBold_->FontSize, ImVec2(p.x + 52 * s_, p.y + (hh - fontBold_->FontSize) / 2), col, names[i]);
        if (i == System && warns) dl->AddCircleFilled(ImVec2(p.x + w - 16 * s_, p.y + hh / 2), 4 * s_, U32(kAmber), 12);
        ImGui::Dummy(ImVec2(0, 0));
        navBottom = p.y + hh;
    }
    float y = navTop.y + Anim("nav.sel", selY, 16.0f);
    dl->ChannelsSetCurrent(0);
    dl->AddRectFilled(ImVec2(navTop.x, y), ImVec2(navTop.x + navW, y + hh), U32(g_chip), g_btnR);
    dl->AddRectFilled(ImVec2(navTop.x + 8 * s_, y + hh / 2 - 9 * s_), ImVec2(navTop.x + 11 * s_, y + hh / 2 + 9 * s_), U32(g_accent), 2 * s_);
    dl->ChannelsMerge();
    marks_["nav"] = ImVec4(navTop.x, navTop.y, navTop.x + navW, navBottom);

    // footer: Send feedback + the panic hint
    float footerH = 44 * s_;
    ImGui::SetCursorPosY(height - footerH - ImGui::GetFrameHeight() - 12 * s_);
    std::string fbLabel = Icon(0xED15);   // Segoe "Feedback"
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x, h = ImGui::GetFrameHeight();
        if (ImGui::InvisibleButton("feedback", ImVec2(w, h))) OpenFeedback();
        bool hov = ImGui::IsItemHovered();
        if (hov) ImGui::SetTooltip("Report a bug, suggest an idea or ask for a game");
        dl = ImGui::GetWindowDrawList();
        dl->AddRect(p, ImVec2(p.x + w, p.y + h), U32(hov ? Lighten(g_line, 0.15f) : g_line), g_btnR);
        dl->AddText(fontIcons_, fontIcons_->FontSize * 0.85f, ImVec2(p.x + 14 * s_, p.y + (h - fontIcons_->FontSize * 0.85f) / 2), U32(hov ? kText : kSub), fbLabel.c_str());
        dl->AddText(ImVec2(p.x + 40 * s_, p.y + (h - ImGui::GetTextLineHeight()) / 2), U32(hov ? kText : kSub), "Send feedback");
        Mark("feedback");
    }
    ImGui::SetCursorPosY(height - footerH);
    ImGui::PushStyleColor(ImGuiCol_Text, kDim);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(hotkey_ ? "Ctrl+Alt+End undoes everything instantly." : "Right-click the tray icon to undo everything.");
    Mark("footer");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

void App::StatusBar() {
    BeginCard("status");
    const GameProfile* a = opt_.Active();
    ImVec4 dotCol = !data_.autoOptimize ? kGray : a ? (a->antiCheat ? kAmber : kGreen) : g_accent;
    std::string title, sub;
    if (!data_.autoOptimize) {
        title = "Paused";
        sub = runningGames_.empty() ? "Auto-optimize is off. Games run with normal Windows settings."
                                    : *runningGames_.begin() + " is running with normal Windows settings (auto-optimize is off).";
    } else if (a) {
        title = "Optimizing: " + a->name;
        sub = opt_.Summary(*a) + "  |  " + util::FormatDuration(opt_.SessionMinutes());
    } else {
        title = "Watching for games";
        sub = "Launch a game from here or anywhere else - it gets optimized automatically.";
    }

    // buttons on the right
    const char* autoLabel = data_.autoOptimize ? "AUTO-OPTIMIZE: ON" : "AUTO-OPTIMIZE: OFF";
    float autoW = ImGui::CalcTextSize(autoLabel).x + ImGui::GetStyle().FramePadding.x * 2;
    std::string upd = updater_.GetState() == Updater::Available ? "Update to v" + updater_.Version()
                    : updater_.GetState() == Updater::Downloading ? "Updating..." : "";
    float updW = upd.empty() ? 0 : ImGui::CalcTextSize(upd.c_str()).x + ImGui::GetStyle().FramePadding.x * 2 + 8 * s_;
    float right = autoW + updW + 8 * s_;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float lineH = ImGui::GetTextLineHeight();
    dl->AddCircleFilled(ImVec2(p.x + 7 * s_, p.y + lineH + 2 * s_), 7 * s_, U32(dotCol), 24);
    float startX = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(startX + 28 * s_);
    ImGui::BeginGroup();
    ImGui::PushFont(fontBold_);
    ImGui::SetWindowFontScale(1.1f);
    ImGui::TextUnformatted(title.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::PushClipRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x - right, ImGui::GetCursorScreenPos().y + lineH + 4), true);
    ImGui::TextColored(kSub, "%s", sub.c_str());
    ImGui::PopClipRect();
    ImGui::EndGroup();

    float y = ImGui::GetItemRectMin().y - ImGui::GetWindowPos().y + (ImGui::GetItemRectSize().y - ImGui::GetFrameHeight()) / 2;
    float x = ImGui::GetWindowContentRegionMax().x - right + 8 * s_;
    ImGui::SetCursorPos(ImVec2(x, y));
    if (!upd.empty()) {
        ImGui::BeginDisabled(updater_.GetState() != Updater::Available);
        if (AccentButton(upd.c_str(), ImVec2(0, 0))) {
            std::string notes = util::Trim(updater_.Notes());
            if (notes.size() > 700) notes = notes.substr(0, 700) + "...";
            std::string m = "Update Project OptM from v" OPTM_VERSION " to v" + updater_.Version() + "?";
            if (!notes.empty()) m += "\n\nWhat's new:\n" + notes;
            m += "\n\nThe app will restart. Your profiles, shortcuts and settings are kept.";
            if (Msg(hwnd_, m, "Project OptM", MB_YESNO | MB_ICONQUESTION) == IDYES) updater_.Install();
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 8 * s_);
    }
    bool on = data_.autoOptimize;
    if (on ? AccentButton(autoLabel, ImVec2(0, 0)) : ImGui::Button(autoLabel)) ToggleAuto();
    EndCard();
    Mark("status");
}

// ------------------------------------------------------------ Home
void App::PerfCard() {
    BeginCard("perf");
    const auto& buf = frames_.Buffer();
    frames::Live st = frames::Stats(buf);
    bool live = (opt_.Active() || frames_.Running()) && buf.size() >= 10;

    // header: big FPS + side stats + toggle
    Label("PERFORMANCE");
    float top = ImGui::GetCursorPosY();
    ImGui::PushFont(fontBig_);
    if (live) ImGui::Text("%d", (int)std::lround(st.fps)); else ImGui::TextUnformatted("--");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, 6 * s_));
    ImGui::TextColored(g_accent, "FPS");
    ImGui::EndGroup();
    ImGui::SameLine(0, 28 * s_);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, 4 * s_));
    if (live && st.low1 > 0) ImGui::TextColored(kSub, "1%% low      %d", (int)std::lround(st.low1)); else ImGui::TextColored(kSub, "1%% low      --");
    if (live) ImGui::TextColored(kSub, "Frametime   %.1f ms", st.frametime); else ImGui::TextColored(kSub, "Frametime   --");
    ImGui::EndGroup();
    const char* tl = data_.fpsOn ? "Graph: ON" : "Graph: OFF";
    const char* ol = data_.overlayOn ? "Overlay: ON" : "Overlay: OFF";
    float pad = ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(tl).x - ImGui::CalcTextSize(ol).x - pad * 2 - 8 * s_);
    ImGui::SetCursorPosY(top);
    if (ImGui::Button(ol)) ToggleOverlay();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Shows FPS, 1%% low and frametime in a corner of your game (Ctrl+Alt+O).\nMore options on the Overlay page.");
    ImGui::SameLine(0, 8 * s_);
    if (ImGui::Button(tl)) {
        data_.fpsOn = !data_.fpsOn;
        data_.SaveConfig();
        if (data_.fpsOn && opt_.Active()) SessionStarted(*opt_.Active());
        if (!data_.fpsOn) frames_.Stop();
    }
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), top + fontBig_->FontSize + 10 * s_));

    // graph
    float w = ImGui::GetContentRegionAvail().x, h = 170 * s_;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(g_card2), g_btnR);
    ImGui::Dummy(ImVec2(w, h));

    std::string msg;
    if (!data_.fpsOn) msg = "FPS graph is off";
    else if (!opt_.Active() && !frames_.Running()) msg = "Start a game to see live FPS and frametimes";
    else if (buf.size() < 10) {
        std::string err = frames_.LastError();
        std::string game = opt_.Active() ? opt_.Active()->name : "this game";
        if (frames_.Blocked()) msg = "No FPS for " + game + " - its anti-cheat blocks frame capture. It's still being optimized.";
        else msg = !frames_.Running() && !err.empty() ? "FPS capture stopped: " + err : "Waiting for frames...";
    }
    if (!msg.empty()) {
        ImVec2 ts = ImGui::CalcTextSize(msg.c_str());
        dl->AddText(ImVec2(p.x + (w - ts.x) / 2, p.y + (h - ts.y) / 2), U32(kDim), msg.c_str());
        EndCard();
        return;
    }

    int maxHz = sys_.dispMaxHz > 0 ? sys_.dispMaxHz : 60;
    double targetMs = 1000.0 / maxHz;
    double lowMs = st.low1 > 0 ? 1000.0 / st.low1 : st.frametime;
    double need = std::max(lowMs * 1.3, std::max(targetMs * 1.6, st.frametime * 1.8));
    double ymax = 200;
    for (double n : { 8.33, 16.67, 33.33, 50.0, 100.0, 200.0 }) if (n >= need) { ymax = n; break; }
    float padY = 6 * s_, plotH = h - padY * 2;
    auto toY = [&](double ms) { return p.y + padY + plotH - (float)(std::min(ms, ymax) / ymax) * plotH; };

    auto refLine = [&](double ms, const char* label) {
        float y = toY(ms);
        for (float x = p.x; x < p.x + w; x += 8 * s_) dl->AddLine(ImVec2(x, y), ImVec2(std::min(x + 4 * s_, p.x + w), y), U32(g_line), 1.0f);
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x + w - ts.x - 6 * s_, std::max(p.y, y - ts.y - 1)), U32(kDim), label);
    };
    char b[32];
    if (targetMs < ymax) { snprintf(b, sizeof(b), "%d Hz", maxHz); refLine(targetMs, b); }
    if (maxHz != 60 && 16.67 < ymax) refLine(16.67, "60 FPS");
    snprintf(b, sizeof(b), "%.4g ms", ymax);
    dl->AddText(ImVec2(p.x + 6 * s_, p.y + 2 * s_), U32(Alpha(kDim, 0.7f)), b);

    int cols = std::max(20, (int)(w / (2 * s_)));
    auto c = frames::Columns(buf, 8000, cols);
    float step = w / (cols - 1);
    std::vector<ImVec2> pts;
    for (int i = 0; i < cols; i++) if (c[i] > 0) pts.push_back(ImVec2(p.x + i * step, toY(c[i])));
    if (pts.size() >= 2) {
        ImU32 soft = U32(Alpha(g_accent, 0.22f));
        float bottom = p.y + h;
        for (size_t i = 1; i < pts.size(); i++)
            dl->AddQuadFilled(pts[i - 1], pts[i], ImVec2(pts[i].x, bottom), ImVec2(pts[i - 1].x, bottom), soft);
        dl->AddPolyline(pts.data(), (int)pts.size(), U32(g_accent), 0, 1.6f * s_);
    }
    EndCard();
}

void App::PageHome() {
    // just what's happening now - sessions, overlay and hardware each have their own page
    PerfCard();
    Mark("perf");
}

// ------------------------------------------------------------ Sessions
void App::PageSessions() {
    float avail = ImGui::GetContentRegionAvail().x;
    float right = 300 * s_;

    ImGui::BeginChild("sessL", ImVec2(avail - right - 14 * s_, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    BeginCard("recent");
    std::string query = util::Lower(util::Trim(sessionSearch_));
    float searchW = 240 * s_;
    ImGui::AlignTextToFramePadding();
    Label("ALL SESSIONS");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - searchW);
    ImGui::SetNextItemWidth(searchW);
    ImGui::InputTextWithHint("##sesssearch", "Search by game", sessionSearch_, sizeof(sessionSearch_));
    ImGui::Dummy(ImVec2(0, 4 * s_));
    if (data_.history.empty()) ImGui::TextColored(kDim, "No sessions yet - play a game and it shows up here.");
    int shown = 0;
    for (int i = (int)data_.history.size() - 1; i >= 0; i--) {
        const Session& ses = data_.history[i];
        if (!query.empty() && util::Lower(ses.game).find(query) == std::string::npos) continue;
        shown++;
        ImGui::PushID(i);
        ImVec2 top = ImGui::GetCursorScreenPos();
        float rowW = ImGui::GetContentRegionAvail().x;
        ImGui::BeginGroup();
        ImGui::PushFont(fontBold_);
        ImGui::TextUnformatted(ses.game.c_str());
        ImGui::PopFont();
        std::string sub = PrettyDate(ses.date);
        if (ses.avgFps > 0) {
            char b[64]; snprintf(b, sizeof(b), "   |   avg %.0f FPS, 1%% low %.0f", ses.avgFps, ses.low1); sub += b;
            // compared with that game's previous session that had FPS
            for (int j = i - 1; j >= 0; j--)
                if (data_.history[j].game == ses.game && data_.history[j].avgFps > 0 && data_.history[j].minutes >= 2) {
                    if (ses.minutes >= 2) sub += "  (" + Signed(ses.avgFps - data_.history[j].avgFps) + ")";
                    break;
                }
        }
        ImGui::TextColored(kDim, "%s", sub.c_str());
        ImGui::EndGroup();
        float rowBottom = ImGui::GetItemRectMax().y;   // the two-line name + date block
        std::string dur = util::FormatDuration(ses.minutes);
        ImGui::SameLine(ImGui::GetCursorStartPos().x + rowW - ImGui::CalcTextSize(dur.c_str()).x);
        ImGui::TextColored(kSub, "%s", dur.c_str());
        // the whole row opens that game's history
        ImVec2 bottom(top.x + rowW, std::max(rowBottom, ImGui::GetItemRectMax().y));
        ImGui::SetCursorScreenPos(top);
        if (ImGui::InvisibleButton("row", ImVec2(rowW, bottom.y - top.y))) historyGame_ = ses.game;
        if (ImGui::IsItemHovered()) {
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(top.x - 6 * s_, top.y - 2 * s_), ImVec2(bottom.x + 6 * s_, bottom.y + 2 * s_),
                                                      U32(Alpha(g_chip, 0.45f)), g_btnR);
            ImGui::SetTooltip("%s - every session and its FPS trend", ses.game.c_str());
        }
        ImGui::Dummy(ImVec2(0, 2 * s_));
        ImGui::PopID();
    }
    if (!data_.history.empty() && !shown) ImGui::TextColored(kDim, "No sessions for a game matching that.");
    EndCard();
    ImGui::EndChild();

    ImGui::SameLine(0, 14 * s_);
    ImGui::BeginChild("sessR", ImVec2(right, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    BeginCard("totals");
    Label("ALL TIME");
    ImGui::Dummy(ImVec2(0, 2 * s_));
    double total = 0;
    for (auto& [g, m] : data_.playtime) total += m;
    ImGui::PushFont(fontBold_);
    ImGui::TextUnformatted(util::FormatHours(total).c_str());
    ImGui::PopFont();
    ImGui::TextColored(kSub, "played over %s, %s", Plural(data_.history.size(), "session").c_str(), Plural(data_.playtime.size(), "game").c_str());
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("top");
    Label("MOST PLAYED");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    std::vector<std::pair<std::string, double>> topList(data_.playtime.begin(), data_.playtime.end());
    std::sort(topList.begin(), topList.end(), [](auto& x, auto& y) { return x.second > y.second; });
    if (topList.empty()) ImGui::TextColored(kDim, "Nothing yet");
    double mx = topList.empty() ? 1 : std::max(1.0, topList[0].second);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < topList.size() && i < 8; i++) {
        float w = ImGui::GetContentRegionAvail().x;
        std::string hrs = util::FormatHours(topList[i].second);
        ImGui::TextUnformatted(topList[i].first.c_str());
        ImGui::SameLine(w - ImGui::CalcTextSize(hrs.c_str()).x);
        ImGui::TextColored(kSub, "%s", hrs.c_str());
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 4 * s_), U32(g_chip), 2 * s_);
        dl->AddRectFilled(p, ImVec2(p.x + w * (float)std::max(0.03, topList[i].second / mx), p.y + 4 * s_), U32(g_accent), 2 * s_);
        ImGui::Dummy(ImVec2(w, 10 * s_));
    }
    EndCard();
    ImGui::EndChild();
}
// ------------------------------------------------------------ Games
void App::PageGames() {
    // A-Z, filtered by the search box (game name or exe name)
    std::string query = util::Lower(util::Trim(gameSearch_));
    std::vector<const GameProfile*> shown;
    for (auto& p : data_.profiles) {
        bool match = query.empty() || util::Lower(p.name).find(query) != std::string::npos;
        for (auto& e : p.exes) if (!match && util::Lower(e).find(query) != std::string::npos) match = true;
        if (match) shown.push_back(&p);
    }
    std::sort(shown.begin(), shown.end(), [](const GameProfile* a, const GameProfile* b) {
        std::string x = util::Lower(a->name), y = util::Lower(b->name);
        return x != y ? x < y : a->name < b->name;
    });

    // header: count, search, Edit profiles
    ImVec2 gamesTop = ImGui::GetCursorScreenPos();
    float searchW = 300 * s_;
    float editW = ImGui::CalcTextSize("Edit profiles").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::AlignTextToFramePadding();
    if (query.empty()) ImGui::TextColored(kSub, "%zu games, A to Z. Launch from here or anywhere else - each game is optimized automatically.", data_.profiles.size());
    else ImGui::TextColored(kSub, "%zu of %zu games match \"%s\"", shown.size(), data_.profiles.size(), util::Trim(gameSearch_).c_str());
    ImGui::SameLine(ImGui::GetContentRegionMax().x - searchW - editW - 8 * s_);
    ImGui::SetNextItemWidth(searchW);
    if (focusGameSearch_) { ImGui::SetKeyboardFocusHere(); focusGameSearch_ = false; }
    ImGui::InputTextWithHint("##gamesearch", "Search games  (Ctrl+F)", gameSearch_, sizeof(gameSearch_));
    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) gameSearch_[0] = 0;
    ImGui::SameLine(0, 8 * s_);
    if (ImGui::Button("Edit profiles")) OpenProfiles();
    ImGui::Dummy(ImVec2(0, 4 * s_));
    if (shown.empty()) {
        ImGui::Dummy(ImVec2(0, 20 * s_));
        ImGui::TextColored(kDim, query.empty() ? "No games yet - start one and it's added automatically, or click Edit profiles."
                                               : "No game matches that. Try part of the name or the exe name.");
        return;
    }

    float avail = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, std::min(4, (int)(avail / (300 * s_))));
    float gap = 12 * s_;
    float tileW = (avail - gap * (cols - 1)) / cols;
    float tileH = 158 * s_;
    for (size_t i = 0; i < shown.size(); i++) {
        const GameProfile& p = *shown[i];
        if (i % cols) ImGui::SameLine(0, gap);
        ImGui::PushID(p.name.c_str());
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * s_, 14 * s_));
        ImGui::BeginChild("tile", ImVec2(tileW, tileH), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        bool running = runningGames_.count(p.name) > 0;
        bool active = opt_.Active() && opt_.Active()->name == p.name;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 cp = ImGui::GetCursorScreenPos();
        dl->AddCircleFilled(ImVec2(cp.x + 5 * s_, cp.y + ImGui::GetTextLineHeight() / 2 + 1), 5 * s_, U32(running ? (active && p.antiCheat ? kAmber : kGreen) : kIdle), 16);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 18 * s_);
        ImGui::PushFont(fontBold_);
        ImGui::TextUnformatted(p.name.c_str());
        ImGui::PopFont();
        bool autoAdded = util::Contains(data_.autoAdded, p.name);
        if (autoAdded) {
            ImGui::SameLine(0, 8 * s_);
            ImGui::SetWindowFontScale(0.8f);
            ImGui::TextColored(g_accent, "NEW");
            ImGui::SetWindowFontScale(1.0f);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Found automatically when it started");
        }
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(kSub, "%s", opt_.Summary(p).c_str());
        ImGui::PopTextWrapPos();
        auto pt = data_.playtime.find(p.name);
        auto lp = data_.launchPaths.find(p.name);
        std::string info = "No launcher set - PLAY asks for one";
        if (lp != data_.launchPaths.end()) {
            size_t slash = lp->second.find_last_of("\\/");
            info = "Launches: " + (slash == std::string::npos ? lp->second : lp->second.substr(slash + 1));
        }
        if (pt != data_.playtime.end() && pt->second >= 1) info += "   |   " + util::FormatHours(pt->second) + " played";
        ImGui::TextColored(kDim, "%s", info.c_str());

        // buttons along the bottom: PLAY  [settings] [plan] [history] [not a game]
        ImGui::SetCursorPosY(tileH - 14 * s_ - ImGui::GetFrameHeight());
        float iconW = ImGui::GetFrameHeight() + 10 * s_;
        int icons = autoAdded ? 4 : 3;
        float playW = ImGui::GetContentRegionAvail().x - iconW * icons - 8 * s_ * icons;
        ImGui::BeginDisabled(running);
        if (AccentButton(running ? "RUNNING" : "PLAY", ImVec2(playW, 0))) StartGame(p.name);
        ImGui::EndDisabled();
        ImGui::SameLine(0, 8 * s_);
        ImGui::PushFont(fontIcons_);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, ImGui::GetStyle().FramePadding.y));
        if (ImGui::Button(Icon(0xE713).c_str(), ImVec2(iconW, ImGui::GetFrameHeight()))) OpenGameSettings(p.name);
        ImGui::PopFont();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Game settings - priority, cores, tweaks, launcher");
        ImGui::SameLine(0, 8 * s_);
        ImGui::PushFont(fontIcons_);
        if (ImGui::Button(Icon(0xE946).c_str(), ImVec2(iconW, ImGui::GetFrameHeight()))) ShowPlan(p);
        ImGui::PopFont();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("What this profile does on your PC");
        ImGui::SameLine(0, 8 * s_);
        ImGui::PushFont(fontIcons_);
        if (ImGui::Button(Icon(0xE9D2).c_str(), ImVec2(iconW, ImGui::GetFrameHeight()))) historyGame_ = p.name;
        ImGui::PopFont();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Session history");
        std::string removeGame;
        if (autoAdded) {
            ImGui::SameLine(0, 8 * s_);
            ImGui::PushFont(fontIcons_);
            if (ImGui::Button(Icon(0xE711).c_str(), ImVec2(iconW, ImGui::GetFrameHeight()))) removeGame = p.name;
            ImGui::PopFont();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Not a game - remove it and never detect it again");
        }
        ImGui::PopStyleVar();
        if (!removeGame.empty()) {   // after drawing: the profile list changes
            ImGui::EndChild();
            ImGui::PopID();
            NotAGame(removeGame);
            return;
        }
        ImGui::EndChild();
        ImGui::PopID();
        if (i == std::min((size_t)cols, shown.size()) - 1)   // tour: the header plus the first row of games
            marks_["games"] = ImVec4(gamesTop.x, gamesTop.y, gamesTop.x + avail, ImGui::GetItemRectMax().y);
        if (i % cols == (size_t)cols - 1) ImGui::Dummy(ImVec2(0, gap - ImGui::GetStyle().ItemSpacing.y));
    }
}

// Per-game history: every session, FPS trend and how the last one compares
void App::HistoryPopup() {
    if (!historyGame_.empty() && !ImGui::IsPopupOpen("history")) ImGui::OpenPopup("history");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(std::min(760 * s_, io.DisplaySize.x - 60 * s_), std::min(560 * s_, io.DisplaySize.y - 60 * s_)));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * s_, 16 * s_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, g_cardR);
    bool open = true;
    if (!ImGui::BeginPopupModal("history", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::PopStyleVar(2);
        return;
    }
    ImGui::PopStyleVar(2);
    std::vector<const Session*> rows;
    for (auto& s : data_.history) if (s.game == historyGame_) rows.push_back(&s);
    double total = 0; int withFps = 0; double best = 0;
    for (auto* r : rows) { total += r->minutes; if (r->avgFps > 0) { withFps++; best = std::max(best, r->avgFps); } }

    ImGui::PushFont(fontTitle_);
    ImGui::TextUnformatted(historyGame_.c_str());
    ImGui::PopFont();
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize("Close").x - ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape)) { historyGame_.clear(); ImGui::CloseCurrentPopup(); }
    std::string head = Plural(rows.size(), "session") + "   |   " + util::FormatHours(total) + " played";
    if (best > 0) head += "   |   best average " + std::to_string((int)std::lround(best)) + " FPS";
    ImGui::TextColored(kSub, "%s", head.c_str());
    ImGui::Dummy(ImVec2(0, 6 * s_));

    // FPS trend: average bars with the 1% low marked, last 30 sessions that had FPS
    std::vector<const Session*> fps;
    for (auto* r : rows) if (r->avgFps > 0) fps.push_back(r);
    if (fps.size() > 30) fps.erase(fps.begin(), fps.end() - 30);
    Label("FPS PER SESSION");
    float w = ImGui::GetContentRegionAvail().x, h = 150 * s_;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), U32(g_card2), g_btnR);
    ImGui::Dummy(ImVec2(w, h));
    if (fps.empty()) {
        const char* m = "No FPS recorded yet - it's captured while you play with the graph on.";
        ImVec2 ts = ImGui::CalcTextSize(m);
        dl->AddText(ImVec2(p.x + (w - ts.x) / 2, p.y + (h - ts.y) / 2), U32(kDim), m);
    } else {
        double mx = 0;
        for (auto* r : fps) mx = std::max(mx, r->avgFps);
        mx *= 1.15;
        float padX = 10 * s_, padY = 10 * s_, slot = (w - padX * 2) / fps.size(), bw = std::clamp(slot * 0.6f, 3.0f, 40 * s_);
        for (size_t i = 0; i < fps.size(); i++) {
            float x = p.x + padX + slot * i + (slot - bw) / 2;
            float yAvg = p.y + h - padY - (float)(fps[i]->avgFps / mx) * (h - padY * 2);
            float yLow = p.y + h - padY - (float)(fps[i]->low1 / mx) * (h - padY * 2);
            bool last = i + 1 == fps.size();
            dl->AddRectFilled(ImVec2(x, yAvg), ImVec2(x + bw, p.y + h - padY), U32(Alpha(g_accent, last ? 0.95f : 0.55f)), 2 * s_);
            if (fps[i]->low1 > 0) dl->AddLine(ImVec2(x - 1, yLow), ImVec2(x + bw + 1, yLow), U32(kAmber), 2 * s_);
            if (ImGui::IsMouseHoveringRect(ImVec2(x, p.y), ImVec2(x + bw, p.y + h)))
                ImGui::SetTooltip("%s\navg %.0f FPS, 1%% low %.0f\n%s", PrettyDate(fps[i]->date).c_str(), fps[i]->avgFps, fps[i]->low1, util::FormatDuration(fps[i]->minutes).c_str());
        }
        char b[32]; snprintf(b, sizeof(b), "%.0f", mx);
        dl->AddText(ImVec2(p.x + 6 * s_, p.y + 2 * s_), U32(Alpha(kDim, 0.7f)), b);
        ImGui::TextColored(kDim, "Bars: average FPS   |   amber line: 1%% low   |   latest session highlighted");
        if (fps.size() >= 2) {
            const Session* a = fps[fps.size() - 1];
            const Session* bs = fps[fps.size() - 2];
            ImGui::TextColored(kSub, "Last session vs the one before: %s avg, %s 1%% low",
                               Signed(a->avgFps - bs->avgFps).c_str(), Signed(a->low1 - bs->low1).c_str());
        }
    }
    ImGui::Dummy(ImVec2(0, 6 * s_));

    // every session, newest first
    if (ImGui::BeginTable("sessions", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerH, ImVec2(0, 0))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("When");
        ImGui::TableSetupColumn("Played");
        ImGui::TableSetupColumn("Avg FPS");
        ImGui::TableSetupColumn("1% low");
        ImGui::TableHeadersRow();
        for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
            const Session* r = *it;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(PrettyDate(r->date).c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(util::FormatDuration(r->minutes).c_str());
            ImGui::TableNextColumn(); if (r->avgFps > 0) ImGui::Text("%.0f", r->avgFps); else ImGui::TextColored(kDim, "--");
            ImGui::TableNextColumn(); if (r->low1 > 0) ImGui::Text("%.0f", r->low1); else ImGui::TextColored(kDim, "--");
        }
        ImGui::EndTable();
    }
    ImGui::EndPopup();
    if (!open) historyGame_.clear();
}

// ------------------------------------------------------------ per-game settings
namespace {
void Fill(char* buf, size_t n, const std::vector<std::string>& v) { snprintf(buf, n, "%s", util::Join(v, ", ").c_str()); }
}  // namespace

void App::OpenGameSettings(const std::string& name) {
    for (auto& p : data_.profiles) {
        if (p.name != name) continue;
        editGame_ = name;
        edit_ = p;
        editDirty_ = false;
        editError_.clear();
        Fill(exeBuf_, sizeof(exeBuf_), p.exes);
        Fill(boostBuf_, sizeof(boostBuf_), p.boost);
        Fill(keepBuf_, sizeof(keepBuf_), p.keep);
        Fill(closeBuf_, sizeof(closeBuf_), p.close);
        return;
    }
}

bool App::SaveGameSettings() {
    editError_.clear();
    edit_.exes = util::NameList(exeBuf_);
    edit_.boost = util::NameList(boostBuf_);
    edit_.keep = util::NameList(keepBuf_);
    edit_.close = util::NameList(closeBuf_);
    if (edit_.exes.empty()) { editError_ = "Add at least one exe name - that's how the game is recognised."; return false; }
    for (auto& e : edit_.exes)
        for (auto& p : data_.profiles)
            if (p.name != editGame_ && util::Contains(p.exes, e)) { editError_ = e + " is already used by " + p.name + "."; return false; }
    if (edit_.tweaks == "Custom") {   // the file keeps a tidy, known list
        std::vector<std::string> ids;
        for (auto& t : tweakset::All()) if (util::Contains(edit_.tweakIds, t.id)) ids.push_back(t.id);
        edit_.tweakIds = ids;
    } else edit_.tweakIds.clear();
    if (data_.ProfilesChanged()) ReloadProfiles();   // pick up a hand edit first, so it isn't overwritten
    if (!data_.SaveProfile(edit_)) { editError_ = "Couldn't save profiles.ini - is it open somewhere, or was the game removed?"; return false; }

    bool running = opt_.Active() && opt_.Active()->name == editGame_;
    if (running) ReloadProfiles();   // ends the session; the next check starts it again with the new settings
    else { data_.LoadProfiles(); opt_.SyncLaunchPriority(); }
    opt_.SyncPerGameSettings();
    Log("Saved settings for " + editGame_ + (running ? " - re-applied to the running game" : " - used the next time it starts"));
    return true;
}

void App::DeleteGame(const std::string& name) {
    if (Msg(hwnd_, "Remove " + name + " from your games?\n\nIts play history is kept. If 'Detect new games' is on, it's added back with default settings the next time it starts - use the X on its tile ('Not a game') to stop that.",
            "Project OptM", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    if (opt_.Active() && opt_.Active()->name == name) opt_.EndSession();
    if (data_.ProfilesChanged()) data_.LoadProfiles();
    data_.RemoveProfile(name);
    data_.autoAdded.erase(std::remove(data_.autoAdded.begin(), data_.autoAdded.end(), name), data_.autoAdded.end());
    data_.launchPaths.erase(name);
    data_.SaveConfig();
    data_.LoadProfiles();
    opt_.SyncLaunchPriority();
    Log("Removed " + name + " from your games");
}

void App::GameSettingsPopup() {
    if (editGame_.empty()) return;
    if (!ImGui::IsPopupOpen("gamesettings")) ImGui::OpenPopup("gamesettings");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(std::min(820 * s_, io.DisplaySize.x - 60 * s_), std::min(720 * s_, io.DisplaySize.y - 60 * s_)));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * s_, 16 * s_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, g_cardR);
    if (!ImGui::BeginPopupModal("gamesettings", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleVar(2);
        return;
    }
    ImGui::PopStyleVar(2);
    GameProfile& e = edit_;
    bool close = false, del = false;
    bool running = runningGames_.count(editGame_) > 0;

    // ---- header
    ImGui::PushFont(fontTitle_);
    ImGui::TextUnformatted(editGame_.c_str());
    ImGui::PopFont();
    ImGui::TextColored(kSub, "%s", running ? "Running now - saving re-applies these settings straight away."
                                           : "Settings for this game only. They're used every time it starts.");
    ImGui::Dummy(ImVec2(0, 4 * s_));

    float footerH = ImGui::GetFrameHeight() + 18 * s_;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card2);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * s_, 8 * s_));
    ImGui::BeginChild("gsbody", ImVec2(0, -footerH), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (testScroll_ > 0 && ImGui::GetFrameCount() > 3) ImGui::SetScrollY(testScroll_ * s_);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, g_chip);          // inputs stand out from the darker panel
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Lighten(g_chip, 0.05f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Lighten(g_chip, 0.08f));

    float ctrlW = 300 * s_;
    // a setting: title + note on the left, its control on the right (the caller draws the control)
    auto row = [&](const char* title, const std::string& note, float ctrlH = 0) {
        ImGui::Dummy(ImVec2(0, 6 * s_));
        float x0 = ImGui::GetCursorPosX(), y0 = ImGui::GetCursorPosY(), w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginGroup();
        ImGui::PushFont(fontBold_);
        ImGui::TextUnformatted(title);
        ImGui::PopFont();
        ImGui::PushTextWrapPos(x0 + w - ctrlW - 24 * s_);
        if (!note.empty()) ImGui::TextColored(kDim, "%s", note.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
        float h = ImGui::GetItemRectSize().y;
        if (ctrlH <= 0) ctrlH = ImGui::GetFrameHeight();
        ImGui::SameLine(x0 + w - ctrlW);
        ImGui::SetCursorPosY(y0 + std::max(0.0f, (h - ctrlH) / 2));
        ImGui::SetNextItemWidth(ctrlW);
    };
    auto rule = [&]() {
        ImGui::Dummy(ImVec2(0, 4 * s_));
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(p, ImVec2(p.x + ImGui::GetContentRegionAvail().x, p.y), U32(g_line), 1.0f);
    };
    auto section = [&](const char* name) {
        ImGui::Dummy(ImVec2(0, 10 * s_));
        Label(name);
    };
    auto sw = [&](const char* id, bool& v, bool enabled = true) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ctrlW - 44 * s_);   // switches sit at the right edge
        if (Switch(id, v, enabled)) { v = !v; editDirty_ = true; }
    };
    auto combo = [&](const char* id, const std::string& shown, const std::vector<std::pair<std::string, std::string>>& items,
                     std::string& value) {   // items: value, label
        if (ImGui::BeginCombo(id, shown.c_str())) {
            for (auto& [v, label] : items)
                if (ImGui::Selectable(label.c_str(), v == value)) { value = v; editDirty_ = true; }
            ImGui::EndCombo();
        }
    };

    // ---- GAME
    section("GAME");
    row("Exe names", "Process names, comma separated. Find them in Task Manager > Details while the game runs.");
    if (ImGui::InputText("##exes", exeBuf_, sizeof(exeBuf_))) editDirty_ = true;
    rule();
    row("Anti-cheat safe mode", "Never touch the game process itself - only system tweaks. Turn this on for games with kernel anti-cheat (Vanguard, EAC, BattlEye, Ricochet...).", 24 * s_);
    sw("gs.ac", e.antiCheat);
    rule();
    {
        auto lp = data_.launchPaths.find(editGame_);
        std::string shown = lp == data_.launchPaths.end() ? "Not set - PLAY asks for it" : lp->second;
        row("Launcher", shown);
        float half = (ctrlW - 8 * s_) / 2, y = ImGui::GetCursorPosY();
        if (ImGui::Button("Change...", ImVec2(half, 0))) PickLauncher(editGame_);
        ImGui::SameLine(0, 8 * s_);
        ImGui::SetCursorPosY(y);
        ImGui::BeginDisabled(lp == data_.launchPaths.end());
        if (ImGui::Button("Clear", ImVec2(half, 0))) { data_.launchPaths.erase(editGame_); data_.SaveConfig(); }
        ImGui::EndDisabled();
    }

    // ---- CPU
    section("CPU");
    ImGui::BeginDisabled(e.antiCheat);
    row("Priority", e.antiCheat ? "Not used in anti-cheat safe mode - use launch priority below."
                                : "How much CPU time Windows gives the game over other apps. Above normal is the safe choice; High can starve audio and input on weaker CPUs.");
    combo("##prio", PriorityLabel(e.priority), { { "Normal", "Normal" }, { "AboveNormal", "Above normal" }, { "High", "High" } }, e.priority);
    rule();
    {
        std::string mode = e.cores == "Best" && e.softPin ? "Prefer" : e.cores;
        bool x3d2 = sys_.layout == CpuLayout::DualX3D;
        std::string best = sys_.canPin ? sys_.BestLabel() + " (CPU " + SystemInfo::MaskText(sys_.bestMask) + ")" : "";
        std::vector<std::pair<std::string, std::string>> items = {
            { "Best", "Best cores - locked" }, { "Prefer", "Best cores - preferred (soft)" } };
        if (x3d2) items.push_back({ "Other", "Other CCD - " + sys_.OtherLabel() });
        items.push_back({ "All", "All cores (no pinning)" });
        std::string label = mode;
        for (auto& [v, l] : items) if (v == mode) label = l;
        if (mode == "Other" && !x3d2) label = "Best cores - locked (Other needs a dual-CCD X3D)";
        std::string note;
        if (!sys_.canPin) note = "Your CPU (" + sys_.LayoutText() + ") doesn't need pinning - the game uses every core.";
        else {
            note = "Best = your " + best + ". Locked keeps the game there; preferred steers it there but lets it spill over - better for games that use lots of threads.";
            if (!tweakset::Active(data_, sys_, &e).count("pinning")) note += " The Core pinning tweak is off in this game's tweaks, so this isn't used.";
        }
        row("Cores", note);
        ImGui::BeginDisabled(!sys_.canPin);
        std::string before = mode;
        combo("##cores", sys_.canPin ? label : "All cores", items, mode);
        if (mode != before) {
            e.softPin = mode == "Prefer";
            e.cores = mode == "Prefer" ? "Best" : mode;
        }
        ImGui::EndDisabled();
    }
    rule();
    row("Block efficiency mode", "Stops Windows from throttling the game with efficiency mode (EcoQoS) when it's in the background or on another screen.", 24 * s_);
    sw("gs.eco", e.ecoQosOff, !e.antiCheat);
    ImGui::EndDisabled();
    rule();
    row("Launch priority", "The priority Windows itself starts the game at. Nothing touches the running game, so it's safe with anti-cheat.");
    combo("##launch", PriorityLabel(e.launchPriority), { { "", "Off" }, { "Normal", "Normal" }, { "AboveNormal", "Above normal" }, { "High", "High" } }, e.launchPriority);

    // ---- MEMORY
    section("MEMORY");
    {
        std::string note = "Clears Windows' standby memory every few minutes while you play - helps games that stutter more the longer they run.";
        if (sys_.ramGB >= 48) note += " Not used with 48 GB+ RAM.";
        else if (sys_.ramGB <= 16) note += " Halved on 16 GB PCs (you have " + std::to_string(sys_.ramGB) + " GB).";
        row("RAM cleanup", note);
        if (ImGui::SliderInt("##ram", &e.ramCleanupMins, 0, 60, e.ramCleanupMins ? "every %d min" : "Off")) editDirty_ = true;
    }

    // ---- APPS
    section("APPS");
    row("Boost", "Helper apps set to High priority while this game runs, e.g. OVRServer_x64 for Oculus VR.");
    if (ImGui::InputTextWithHint("##boost", "none", boostBuf_, sizeof(boostBuf_))) editDirty_ = true;
    rule();
    row("Close when it starts", "Apps closed when this game starts (on top of the ones in profiles.ini [Settings]).");
    if (ImGui::InputTextWithHint("##close", "none", closeBuf_, sizeof(closeBuf_))) editDirty_ = true;
    rule();
    row("Keep open", "Launchers this game needs - never closed for it. Groups work: steam, epic, riot, battlenet, ea, oculus, xbox, ubisoft, gog.");
    if (ImGui::InputTextWithHint("##keep", "none", keepBuf_, sizeof(keepBuf_))) editDirty_ = true;

    // ---- TWEAKS
    section("TWEAKS");
    {
        std::vector<std::pair<std::string, std::string>> items = { { "", "Tweaks page preset (" + data_.tweakPreset + ")" } };
        for (auto& b : tweakset::BuiltIns()) items.push_back({ b, b });
        for (auto& [name, ids] : data_.tweakPresets) items.push_back({ name, name });
        items.push_back({ "Custom", "Custom - just for this game" });
        std::string shown = items[0].second;
        for (auto& [v, l] : items) if (v == e.tweaks) shown = l;
        if (!e.tweaks.empty() && e.tweaks != "Custom" && !tweakset::PresetExists(data_, e.tweaks)) shown = e.tweaks + " (deleted - using the Tweaks page preset)";
        auto active = tweakset::Active(data_, sys_, &e);
        row("Preset", std::to_string(active.size()) + " of " + std::to_string(tweakset::All().size()) +
                      " tweaks on for this game. Custom lets you flip each one; anything this PC can't use stays off.");
        std::string before = e.tweaks;
        combo("##tweaks", shown, items, e.tweaks);
        if (e.tweaks == "Custom" && before != "Custom") {   // start from what it had
            GameProfile was = e; was.tweaks = before;
            auto from = tweakset::ChosenFor(data_, &was);
            e.tweakIds.assign(from.begin(), from.end());
        }
        if (e.tweaks == "Custom") {
            auto chosen = tweakset::ChosenFor(data_, &e);
            for (auto* cat : tweakset::Categories()) {
                ImGui::Dummy(ImVec2(0, 8 * s_));
                ImGui::TextColored(kSub, "%s", cat);
                if (!ImGui::BeginTable(cat, 2, ImGuiTableFlags_SizingStretchSame)) continue;
                int col = 0;
                for (auto& t : tweakset::All()) {
                    if (std::string(t.category) != cat) continue;
                    if (col++ % 2 == 0) ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    std::string why = tweakset::Unavailable(t.id, sys_, data_);
                    bool on = chosen.count(t.id) && why.empty();
                    if (Switch((std::string("gst.") + t.id).c_str(), on, why.empty())) {
                        if (on) e.tweakIds.erase(std::remove(e.tweakIds.begin(), e.tweakIds.end(), t.id), e.tweakIds.end());
                        else e.tweakIds.push_back(t.id);
                        editDirty_ = true;
                    }
                    ImGui::SameLine(0, 10 * s_);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (24 * s_ - ImGui::GetTextLineHeight()) / 2);
                    ImGui::TextColored(why.empty() ? kText : kDim, "%s", t.name);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s%s%s", t.desc, why.empty() ? "" : "\n\nUnavailable: ", why.c_str());
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::Dummy(ImVec2(0, 10 * s_));
    ImGui::PopStyleColor(3);
    ImGui::EndChild();

    // ---- footer: Remove   [error]   Cancel  Save
    ImGui::Dummy(ImVec2(0, 6 * s_));
    if (ImGui::Button("Remove game")) del = true;
    if (!editError_.empty()) {
        ImGui::SameLine(0, 16 * s_);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kAmber, "%s", editError_.c_str());
    } else if (editDirty_) {
        ImGui::SameLine(0, 16 * s_);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kDim, "Unsaved changes");
    }
    float saveW = 110 * s_, cancelW = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - saveW - cancelW - 8 * s_);
    bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsAnyItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (ImGui::Button("Cancel") || esc) close = true;
    ImGui::SameLine(0, 8 * s_);
    if (AccentButton("Save", ImVec2(saveW, 0)) && SaveGameSettings()) close = true;

    if (close || del) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
    if (del) { std::string gone = editGame_; editGame_.clear(); DeleteGame(gone); }
    else if (close) editGame_.clear();
}

// ------------------------------------------------------------ feedback
// Feedback becomes a GitHub issue: we fill it in, the user reviews it and clicks Submit in the browser.
// Nothing is sent from the app itself.
namespace {
const char* kFeedbackTypes[] = { "Bug", "Idea", "Game request", "Other" };
const char* kFeedbackHints[] = {
    "What happened, and what did you expect? Steps to make it happen again help a lot.",
    "What would you like Project OptM to do?",
    "Which game, and where do you get it (Steam, Epic...)? Its exe name helps if you know it.",
    "Anything else on your mind.",
};
}

void App::OpenFeedback() {
    feedbackOpen_ = true;
    fbStatus_.clear();
    // about the game that's running, or the last one played
    fbGame_.clear();
    if (opt_.Active()) fbGame_ = opt_.Active()->name;
    else
        for (auto it = data_.history.rbegin(); it != data_.history.rend() && fbGame_.empty(); ++it)
            for (auto& p : data_.profiles) if (p.name == it->game) { fbGame_ = p.name; break; }
}

std::string App::FeedbackTitle() const {
    std::string t = util::Trim(fbTitle_);
    return "[" + std::string(kFeedbackTypes[fbType_]) + "] " + (t.empty() ? std::string("(no title)") : t);
}

std::string App::FeedbackBody() const {
    std::string b;
    std::string details = util::Trim(fbDetails_);
    b += details.empty() ? "_(no details)_" : details;
    b += "\n\n---\n**Project OptM** " OPTM_VERSION;
    if (!fbGame_.empty()) b += "  |  **Game:** " + fbGame_;
    b += "\n";
    auto section = [&](const std::string& title, const std::string& text) {
        b += "\n<details><summary>" + title + "</summary>\n\n```\n" + text + "```\n</details>\n";
    };
    if (fbSpecs_) {
        std::string s;
        for (auto& [k, v] : SpecRows()) s += std::string(*k ? k : "GPU") + ": " + v + "\n";
        s += "Layout: " + sys_.LayoutText() + (sys_.canPin ? ", games pinned to " + sys_.BestLabel() + " (CPU " + SystemInfo::MaskText(sys_.bestMask) + ")" : "") + "\n";
        if (sys_.microcode) { char m[32]; snprintf(m, sizeof(m), "0x%X", sys_.microcode); s += std::string("Microcode: ") + m + "\n"; }
        if (sys_.hasBattery) s += "Laptop (has a battery)\n";
        section("PC specs", s);
    }
    const GameProfile* game = nullptr;
    for (auto& p : data_.profiles) if (p.name == fbGame_) game = &p;
    if (game && fbGameSettings_) {
        const GameProfile& p = *game;
        std::string s = "exe: " + util::Join(p.exes, ", ") + "\n";
        s += "Summary: " + opt_.Summary(p) + "\n";
        if (p.antiCheat) s += "Anti-cheat safe mode: yes\n";
        s += "Priority: " + PriorityLabel(p.priority) + "  |  Cores: " + (p.softPin ? "Prefer" : p.cores) + "  |  Launch priority: " + PriorityLabel(p.launchPriority) + "\n";
        if (p.ramCleanupMins) s += "RAM cleanup: every " + std::to_string(p.ramCleanupMins) + " min\n";
        if (!p.boost.empty()) s += "Boost: " + util::Join(p.boost, ", ") + "\n";
        if (!p.close.empty()) s += "Close: " + util::Join(p.close, ", ") + "\n";
        if (!p.keep.empty()) s += "Keep: " + util::Join(p.keep, ", ") + "\n";
        std::vector<std::string> on;
        for (auto& id : tweakset::Active(data_, sys_, &p)) on.push_back(id);
        s += "Tweaks: " + tweakset::PresetFor(data_, &p) + (p.tweaks.empty() ? " (Tweaks page)" : " (set for this game)") + " - " + util::Join(on, ", ") + "\n";
        section("Game settings", s);
    }
    if (game && fbFps_) {
        const Session* last = nullptr;
        int n = 0;
        for (auto& h : data_.history) if (h.game == fbGame_) { n++; if (h.avgFps > 0) last = &h; }
        std::string s;
        if (last) {
            char f[160];
            snprintf(f, sizeof(f), "Last session with FPS: %s, %s, avg %.0f FPS, 1%% low %.0f\n", last->date.c_str(),
                     util::FormatDuration(last->minutes).c_str(), last->avgFps, last->low1);
            s = f;
        } else s = "No FPS recorded for this game yet\n";
        s += std::to_string(n) + " session(s) in total\n";
        section("FPS", s);
    }
    if (fbLog_ && !log_.empty()) {
        size_t from = log_.size() > 100 ? log_.size() - 100 : 0;
        std::string s;
        for (size_t i = from; i < log_.size(); i++) s += log_[i] + "\n";
        section("Activity log (last " + std::to_string(log_.size() - from) + " lines)", s);
    }
    return util::HidePersonal(b);
}

void App::SendFeedback() {
    std::string title = FeedbackTitle(), body = FeedbackBody();
    std::string base = "https://github.com/" OPTM_UPDATE_REPO "/issues/new?title=" + util::UrlEncode(title) + "&body=";
    std::string url = base + util::UrlEncode(body);
    // GitHub turns away very long links (and signing in wraps the link in another, longer one):
    // then the report goes on the clipboard to paste in
    if (url.size() > 4000) {
        ImGui::SetClipboardText(body.c_str());
        url = base + util::UrlEncode("**Paste your report here (Ctrl+V)** - Project OptM copied it to your clipboard.\n\n");
        fbStatus_ = "Your report is on the clipboard - paste it into the issue with Ctrl+V, then click Submit.";
    } else fbStatus_ = "Opened in your browser - check it over and click Submit new issue.";
    if (!util::EnvVar(L"OPTM_FEEDBACK_TEST").empty()) { Log("Feedback URL (" + std::to_string(url.size()) + " chars): " + url); return; }   // developer check
    util::OpenAsUser(util::Widen(url));
    Log("Feedback opened on GitHub: " + title);
}

void App::FeedbackPopup() {
    if (!feedbackOpen_) return;
    if (!ImGui::IsPopupOpen("feedback")) ImGui::OpenPopup("feedback");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(std::min(760 * s_, io.DisplaySize.x - 60 * s_), std::min(700 * s_, io.DisplaySize.y - 60 * s_)));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * s_, 16 * s_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, g_cardR);
    if (!ImGui::BeginPopupModal("feedback", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleVar(2);
        return;
    }
    ImGui::PopStyleVar(2);
    bool close = false;

    ImGui::PushFont(fontTitle_);
    ImGui::TextUnformatted("Send feedback");
    ImGui::PopFont();
    ImGui::TextColored(kSub, "This becomes an issue on Project OptM's GitHub page. You'll see it in your browser before anything is sent.");
    ImGui::Dummy(ImVec2(0, 4 * s_));

    float footerH = ImGui::GetFrameHeight() + 18 * s_;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card2);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * s_, 12 * s_));
    ImGui::BeginChild("fbbody", ImVec2(0, -footerH), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    if (testScroll_ > 0 && ImGui::GetFrameCount() > 3) ImGui::SetScrollY(testScroll_ * s_);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, g_chip);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Lighten(g_chip, 0.05f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Lighten(g_chip, 0.08f));

    // type chips
    for (int i = 0; i < 4; i++) {
        if (i) ImGui::SameLine(0, 8 * s_);
        bool sel = fbType_ == i;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, sel ? g_accent : g_chip);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sel ? g_accent : Lighten(g_chip, 0.06f));
        ImGui::PushStyleColor(ImGuiCol_Text, sel ? TextOn(g_accent) : kSub);
        if (ImGui::Button(kFeedbackTypes[i])) fbType_ = i;
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
    ImGui::Dummy(ImVec2(0, 6 * s_));
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::TextColored(kSub, "Title");
    ImGui::SetNextItemWidth(w);
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    ImGui::InputTextWithHint("##fbtitle", "A short summary", fbTitle_, sizeof(fbTitle_));
    ImGui::Dummy(ImVec2(0, 4 * s_));
    ImGui::TextColored(kSub, "Details");
    ImGui::InputTextMultiline("##fbdetails", fbDetails_, sizeof(fbDetails_), ImVec2(w, 130 * s_));
    if (!fbDetails_[0] && !ImGui::IsItemActive()) {   // hint text (multiline boxes have none)
        ImVec2 p = ImGui::GetItemRectMin();
        ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + ImGui::GetStyle().FramePadding.x, p.y + ImGui::GetStyle().FramePadding.y), U32(kDim), kFeedbackHints[fbType_]);
    }
    ImGui::Dummy(ImVec2(0, 4 * s_));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kSub, "About a game?");
    ImGui::SameLine(0, 12 * s_);
    ImGui::SetNextItemWidth(300 * s_);
    if (ImGui::BeginCombo("##fbgame", fbGame_.empty() ? "No - the app in general" : fbGame_.c_str())) {
        if (ImGui::Selectable("No - the app in general", fbGame_.empty())) fbGame_.clear();
        std::vector<std::string> names;
        for (auto& p : data_.profiles) names.push_back(p.name);
        std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) { return util::Lower(a) < util::Lower(b); });
        for (auto& n : names) if (ImGui::Selectable(n.c_str(), n == fbGame_)) fbGame_ = n;
        ImGui::EndCombo();
    }

    // what to attach
    ImGui::Dummy(ImVec2(0, 10 * s_));
    Label("ATTACH (OPTIONAL)");
    ImGui::Dummy(ImVec2(0, 2 * s_));
    auto attach = [&](const char* id, bool& v, const char* title, const char* note, bool enabled) {
        ImGui::BeginDisabled(!enabled);
        if (Switch(id, v && enabled, enabled)) v = !v;
        ImGui::EndDisabled();
        ImGui::SameLine(0, 12 * s_);
        ImGui::BeginGroup();
        ImGui::TextColored(enabled ? kText : kDim, "%s", title);
        ImGui::SameLine(0, 8 * s_);
        ImGui::TextColored(kDim, "%s", note);
        ImGui::EndGroup();
    };
    bool hasGame = !fbGame_.empty();
    attach("fb.specs", fbSpecs_, "PC specs", "CPU, GPU, RAM, display, Windows", true);
    attach("fb.game", fbGameSettings_, "Game settings", hasGame ? "priority, cores, tweaks for this game" : "pick a game above", hasGame);
    attach("fb.fps", fbFps_, "FPS of the last session", hasGame ? "average and 1% lows" : "pick a game above", hasGame);
    attach("fb.log", fbLog_, "Recent activity", "the last 100 lines of the Activity page", !log_.empty());
    ImGui::TextColored(kDim, "Your Windows user name is hidden from file paths. Nothing is sent until you click Submit on GitHub.");

    // preview
    ImGui::Dummy(ImVec2(0, 6 * s_));
    if (ImGui::Button(fbPreview_ ? "Hide preview" : "Preview what's sent")) fbPreview_ = !fbPreview_;
    if (fbPreview_) {
        std::string text = FeedbackTitle() + "\n\n" + FeedbackBody();
        ImGui::InputTextMultiline("##fbpreview", text.data(), text.size() + 1, ImVec2(w, 260 * s_), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::PopStyleColor(3);
    ImGui::EndChild();

    // footer: [status]   Copy   Cancel   Open on GitHub
    ImGui::Dummy(ImVec2(0, 6 * s_));
    bool ready = util::Trim(fbTitle_).size() >= 3;
    if (!fbStatus_.empty()) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kGreen, "%s", fbStatus_.c_str());
    } else if (!ready) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kDim, "Add a title to continue");
    }
    const char* sendLabel = "Open on GitHub";
    float sendW = ImGui::CalcTextSize(sendLabel).x + 40 * s_;
    float copyW = ImGui::CalcTextSize("Copy").x + ImGui::GetStyle().FramePadding.x * 2;
    float cancelW = ImGui::CalcTextSize("Close").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - sendW - copyW - cancelW - 16 * s_);
    ImGui::BeginDisabled(!ready);
    if (ImGui::Button("Copy")) {
        std::string all = FeedbackTitle() + "\n\n" + FeedbackBody();
        ImGui::SetClipboardText(all.c_str());
        fbStatus_ = "Copied - paste it wherever you like.";
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Copy the report instead (for Discord, email...)");
    ImGui::SameLine(0, 8 * s_);
    bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsAnyItemActive() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (ImGui::Button("Close") || esc) close = true;
    ImGui::SameLine(0, 8 * s_);
    ImGui::BeginDisabled(!ready);
    if (AccentButton(sendLabel, ImVec2(sendW, 0))) SendFeedback();
    ImGui::EndDisabled();

    if (close) {
        ImGui::CloseCurrentPopup();
        feedbackOpen_ = false;
        if (!fbStatus_.empty() && fbStatus_.rfind("Copied", 0) != 0) { fbTitle_[0] = 0; fbDetails_[0] = 0; }   // sent: start fresh next time
    }
    ImGui::EndPopup();
}

// ------------------------------------------------------------ System
// CPU / GPU / RAM / display / Windows, as shown on the System page ("" label = another GPU)
std::vector<std::pair<const char*, std::string>> App::SpecRows() const {
    char b[160];
    std::vector<std::pair<const char*, std::string>> rows;
    snprintf(b, sizeof(b), "%s  |  %d cores / %d threads  |  ", sys_.cpuName.c_str(), sys_.cores, sys_.threads);
    rows.push_back({ "CPU", b + sys_.LayoutText() });
    for (size_t i = 0; i < sys_.gpus.size(); i++) {
        snprintf(b, sizeof(b), "%s  |  %.0f GB", sys_.gpus[i].name.c_str(), sys_.gpus[i].vramBytes / 1073741824.0);
        rows.push_back({ i == 0 ? "GPU" : "", b });
    }
    std::string ram = std::to_string(sys_.ramGB) + " GB " + sys_.ramType;
    if (!sys_.ramLayout.empty()) ram += " (" + sys_.ramLayout + ")";
    if (sys_.ramMTs) ram += " @ " + std::to_string(sys_.ramMTs) + " MT/s";
    rows.push_back({ "RAM", ram });
    std::string disp = std::to_string(sys_.dispW) + " x " + std::to_string(sys_.dispH);
    if (sys_.dispHz > 1) disp += " @ " + std::to_string(sys_.dispHz) + " Hz";
    if (sys_.dispMaxHz > sys_.dispHz && sys_.dispHz > 1) disp += " (supports " + std::to_string(sys_.dispMaxHz) + " Hz)";
    rows.push_back({ "Display", disp });
    rows.push_back({ "Windows", sys_.osName });
    return rows;
}

void App::PageSystem() {
    BeginCard("specs");
    Label("YOUR SYSTEM");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    auto rows = SpecRows();
    for (auto& [k, v] : rows) {
        ImGui::TextColored(kDim, "%s", k);
        ImGui::SameLine(90 * s_);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(v.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::Dummy(ImVec2(0, 2 * s_));
    if (ImGui::Button("Copy specs")) {
        std::string all;
        for (auto& [k, v] : rows) all += std::string(*k ? k : "GPU") + ": " + v + "\r\n";
        ImGui::SetClipboardText(all.c_str());
        Log("System specs copied to clipboard");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear shader cache")) ClearShaderCache();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fixes stutter or crashes after a graphics driver update");
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("checks");
    Label("HEALTH CHECKS");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize("Re-check").x - ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::SmallButton("Re-check")) { RefreshChecks(); Log("Health checks refreshed"); }
    ImGui::Dummy(ImVec2(0, 4 * s_));
    Chips();
    ImGui::Dummy(ImVec2(0, 2 * s_));
    ImGui::TextColored(kDim, "Hover a check for details. Outlined checks can be fixed with a click.");
    EndCard();
    Mark("checks");
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("tuned");
    Label("TUNED FOR YOUR HARDWARE");
    ImGui::Dummy(ImVec2(0, 6 * s_));
    for (auto& [vendor, text] : HardwareLines()) {
        float x = ImGui::GetCursorPosX();
        VendorBadge(vendor);
        ImGui::SameLine(x + 80 * s_);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Dummy(ImVec2(0, 4 * s_));
    }
    EndCard();
}

// ------------------------------------------------------------ Activity
void App::PageActivity() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_card2);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16 * s_, 12 * s_));
    ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, kSub);
    ImGui::PushTextWrapPos(0);
    for (auto& l : log_) ImGui::TextUnformatted(l.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 5) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

// ------------------------------------------------------------ Overlay
void App::PageOverlay() {
    float w = std::min(ImGui::GetContentRegionAvail().x, 760 * s_);
    ImGui::BeginChild("overlayCol", ImVec2(w, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    auto toggle = [&](const char* label, bool on) { return on ? AccentButton(label, ImVec2(0, 0)) : ImGui::Button(label); };
    BeginCard("overlay");
    Label("IN-GAME OVERLAY");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    if (toggle(data_.overlayOn ? "Overlay: ON" : "Overlay: OFF", data_.overlayOn)) ToggleOverlay();
    ImGui::SameLine();
    if (toggle(data_.overlayAntiCheat ? "Anti-cheat games: ON" : "Anti-cheat games: OFF", data_.overlayAntiCheat)) {
        data_.overlayAntiCheat = !data_.overlayAntiCheat;
        data_.SaveConfig();
        Log(data_.overlayAntiCheat ? "The overlay now also shows over anti-cheat games" : "The overlay is hidden over anti-cheat games");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The overlay is a separate window - nothing is loaded into the game - but some anti-cheats\n"
                          "watch for windows drawn over their game. Off (the default) keeps it hidden in those games.");
    // what it shows
    ImGui::Dummy(ImVec2(0, 2 * s_));
    ImGui::TextColored(kSub, "Show under the FPS");
    for (size_t i = 0; i < sizeof(kOverlayItems) / sizeof(kOverlayItems[0]); i++) {
        if (i) ImGui::SameLine();
        if (ImGui::GetCursorPosX() + ImGui::CalcTextSize(kOverlayItems[i].second).x + 30 * s_ > ImGui::GetContentRegionMax().x) ImGui::NewLine();
        bool on = util::Contains(data_.overlayItems, kOverlayItems[i].first);
        if (toggle(kOverlayItems[i].second, on)) {
            std::vector<std::string> items;   // kept in display order
            for (auto& [id, label] : kOverlayItems)
                if (id == std::string(kOverlayItems[i].first) ? !on : util::Contains(data_.overlayItems, id)) items.push_back(id);
            data_.overlayItems = items;
            data_.SaveConfig();
        }
    }
    if (sys_.gpus.empty()) ImGui::TextColored(kDim, "No graphics card found - GPU and VRAM show --");

    // look: background and size
    ImGui::Dummy(ImVec2(0, 2 * s_));
    float sw = 220 * s_;
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kSub, "Background");
    ImGui::SameLine(110 * s_);
    ImGui::SetNextItemWidth(sw);
    ImGui::SliderInt("##ovop", &data_.overlayOpacity, 0, 100, data_.overlayOpacity == 0 ? "See-through" : "%d%%");
    if (ImGui::IsItemDeactivatedAfterEdit()) data_.SaveConfig();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("0%% = no background, just outlined text over the game");
    ImGui::SameLine(0, 24 * s_);
    ImGui::TextColored(kSub, "Size");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(sw);
    ImGui::SliderInt("##ovsize", &data_.overlaySize, 70, 160, "%d%%");
    if (ImGui::IsItemDeactivatedAfterEdit()) data_.SaveConfig();

    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(kDim, "Shown while you're in the game. %s shows or hides it. It's a separate click-through window "
                             "(nothing is loaded into the game), so it shows over windowed, borderless and most modern "
                             "fullscreen games, but not old exclusive fullscreen. Temperature shows if your graphics driver reports it.",
                       overlayHotkey_ ? "Ctrl+Alt+O" : "The switch above (Ctrl+Alt+O is taken by another app)");
    ImGui::PopTextWrapPos();
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    // where it sits: a picture of your screen with the overlay in it - drag it, or snap to a corner
    BeginCard("position");
    Label("POSITION");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    OverlayPositioner();
    ImGui::Dummy(ImVec2(0, 4 * s_));
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kSub, "Snap to");
    ImGui::SameLine(0, 16 * s_);
    const char* corners[] = { "Top left", "Top right", "Bottom left", "Bottom right" };
    for (int i = 0; i < 4; i++) {
        if (i) ImGui::SameLine();
        double cx = (i == 1 || i == 3) ? 1 : 0, cy = i >= 2 ? 1 : 0;
        if (toggle(corners[i], data_.overlayX == cx && data_.overlayY == cy)) {
            data_.overlayX = cx; data_.overlayY = cy;
            data_.SaveConfig();
        }
    }
    EndCard();
    ImGui::EndChild();
}

// A picture of your screen (its real shape) with the overlay drawn inside at its real size and spot.
// Drag the overlay to move it, or click anywhere on the screen to put it there.
void App::OverlayPositioner() {
    HWND anchor = opt_.Active() ? GameWindow() : nullptr;   // the game's screen if one's running, else this window's
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(anchor ? anchor : hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& mon = mi.rcMonitor;
    float monW = (float)std::max(1L, mon.right - mon.left), monH = (float)std::max(1L, mon.bottom - mon.top);
    // (on another monitor Windows' own scaling may differ - close enough for placing it)

    float boxW = std::min(ImGui::GetContentRegionAvail().x, 640 * s_), boxH = boxW * monH / monW;
    float k = boxW / monW;   // screen pixels -> preview pixels
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("screen", ImVec2(boxW, boxH));
    bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // the screen: dark, with a faint "game" gradient so the see-through setting shows
    float r = 6 * s_;
    dl->AddRectFilledMultiColor(p, ImVec2(p.x + boxW, p.y + boxH), U32(Hex("#1B2536")), U32(Hex("#2A2340")), U32(Hex("#1A1F2B")), U32(Hex("#11151D")));
    dl->AddRect(p, ImVec2(p.x + boxW, p.y + boxH), U32(hovered || active ? Lighten(g_line, 0.25f) : g_line), r, 0, 2 * s_);
    char res[48]; snprintf(res, sizeof(res), "%d x %d", (int)monW, (int)monH);
    ImVec2 rs = ImGui::CalcTextSize(res);
    dl->AddText(ImVec2(p.x + boxW - rs.x - 10 * s_, p.y + boxH - rs.y - 8 * s_), U32(Alpha(kSub, 0.6f)), res);

    // the overlay, at its real size and spot
    OverlayContent c = OverlayNow();
    float sc = OverlayScale();
    SIZE real = Overlay::SizeFor(c.stats.size(), c.showGraph, sc);
    float ow = std::max(18 * s_, real.cx * k), oh = std::max(12 * s_, real.cy * k), m = Overlay::Margin(sc) * k;
    float spanX = std::max(1.0f, boxW - ow - 2 * m), spanY = std::max(1.0f, boxH - oh - 2 * m);
    ImVec2 o(p.x + m + (float)data_.overlayX * spanX, p.y + m + (float)data_.overlayY * spanY);
    ImGuiIO& io = ImGui::GetIO();
    bool overIt = io.MousePos.x >= o.x && io.MousePos.x <= o.x + ow && io.MousePos.y >= o.y && io.MousePos.y <= o.y + oh;
    if (ImGui::IsItemActivated())   // grabbed it: keep that point under the mouse; clicked elsewhere: centre it there
        overlayGrab_ = overIt ? ImVec2(io.MousePos.x - o.x, io.MousePos.y - o.y) : ImVec2(ow / 2, oh / 2);
    if (active) {
        data_.overlayX = std::clamp((io.MousePos.x - overlayGrab_.x - p.x - m) / spanX, 0.0f, 1.0f);
        data_.overlayY = std::clamp((io.MousePos.y - overlayGrab_.y - p.y - m) / spanY, 0.0f, 1.0f);
        o = ImVec2(p.x + m + (float)data_.overlayX * spanX, p.y + m + (float)data_.overlayY * spanY);
    }
    if (ImGui::IsItemDeactivated()) { data_.SaveConfig(); Log("Overlay position saved"); }
    if (hovered || active) ImGui::SetMouseCursor(overIt || active ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_Hand);

    // a small copy of the overlay: background at its opacity, the FPS in the accent color, the rest as lines
    ImVec2 oe(o.x + ow, o.y + oh);
    float orad = std::min(10 * sc * k, oh / 3);
    if (c.opacity > 0) dl->AddRectFilled(o, oe, U32(ImVec4(0.047f, 0.051f, 0.067f, c.opacity / 100.0f)), orad);
    dl->AddRect(o, oe, U32(Alpha(g_accent, active || overIt ? 1.0f : 0.7f)), orad, 0, (active ? 2.0f : 1.5f) * s_);
    ImVec4 accent(GetRValue(c.accent) / 255.f, GetGValue(c.accent) / 255.f, GetBValue(c.accent) / 255.f, 1);
    float pad = 12 * sc * k;
    char fps[16]; snprintf(fps, sizeof(fps), "%s", c.hasFps ? std::to_string((int)std::lround(c.fps)).c_str() : "FPS");
    float fsz = std::max(8 * s_, 26 * sc * k);
    dl->AddText(fontBold_, fsz, ImVec2(o.x + pad, o.y + pad * 0.5f), U32(accent), fps);
    float ly = o.y + (12 + 34) * sc * k, rowH = 19 * sc * k, colW = (ow - 2 * pad) / 2;
    for (size_t i = 0; i < c.stats.size(); i++) {
        float lx = o.x + pad + (i % 2) * colW, yy = ly + (i / 2) * rowH + rowH * 0.35f;
        dl->AddRectFilled(ImVec2(lx, yy), ImVec2(lx + colW * 0.75f, yy + std::max(1.5f, rowH * 0.3f)), U32(Alpha(kSub, 0.8f)), 1);
    }
    if (c.showGraph) {
        float gy = ly + ((c.stats.size() + 1) / 2) * rowH + 6 * sc * k, gh = 20 * sc * k;
        dl->AddRectFilled(ImVec2(o.x + pad, gy), ImVec2(oe.x - pad, gy + gh), U32(Alpha(kGreen, 0.55f)), 1);
    }

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + boxW);
    ImGui::TextColored(kDim, "Your screen%s - drag the overlay where you want it, or click anywhere to move it there. "
                             "It lands in the same spot in every game.", anchor ? " (the game's monitor)" : "");
    ImGui::PopTextWrapPos();
}
// ------------------------------------------------------------ Settings
void App::PageSettings() {
    float w = std::min(ImGui::GetContentRegionAvail().x, 680 * s_);
    ImGui::BeginChild("settingsCol", ImVec2(w, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    auto toggle = [&](const char* label, bool on) { return on ? AccentButton(label, ImVec2(0, 0)) : ImGui::Button(label); };

    BeginCard("optimizing");
    Label("OPTIMIZING");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    if (toggle(data_.autoOptimize ? "Auto-optimize: ON" : "Auto-optimize: OFF", data_.autoOptimize)) ToggleAuto();
    ImGui::SameLine();
    if (toggle(data_.autoDetect ? "Detect new games: ON" : "Detect new games: OFF", data_.autoDetect)) {
        data_.autoDetect = !data_.autoDetect;
        data_.SaveConfig();
        Log(data_.autoDetect ? "Detecting new games when they start" : "New-game detection off - only games in your profiles are optimized");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("When a game without a profile starts (from Steam, Epic, GOG, Ubisoft, Xbox, EA, Riot or Windows' game list),\nit's added to your games and optimized automatically.");
    ImGui::SameLine();
    if (toggle(data_.fpsOn ? "FPS graph: ON" : "FPS graph: OFF", data_.fpsOn)) {
        data_.fpsOn = !data_.fpsOn;
        data_.SaveConfig();
        if (data_.fpsOn && opt_.Active()) SessionStarted(*opt_.Active());
        if (!data_.fpsOn) frames_.Stop();
    }
    ImGui::SameLine();
    if (ImGui::Button("Panic: undo everything")) Panic();
    if (toggle(data_.revertOnExit ? "Restore everything on exit: ON" : "Restore everything on exit: OFF", data_.revertOnExit)) {
        data_.revertOnExit = !data_.revertOnExit;
        data_.SaveConfig();
        Log(data_.revertOnExit ? "Exiting now puts every setting back, including launch priority, GPU preference and fullscreen optimizations"
                               : "Launch priority, GPU preference and fullscreen optimizations now stay set after you exit");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("ON: when you exit Project OptM, your PC goes back exactly to how it was - including the per-game\n"
                          "launch priority, GPU preference and fullscreen optimization settings. They're set again when it starts.\n"
                          "OFF: those three stay set, so they work even while Project OptM isn't running.");
    ImGui::SameLine();
    bool testCopy = !util::EnvVar(L"OPTM_DATA_DIR").empty();
    ImGui::BeginDisabled(testCopy);
    if (toggle(startWithWindows_ ? "Start with Windows: ON" : "Start with Windows: OFF", startWithWindows_)) {
        std::string err;
        bool on = !startWithWindows_;
        if (on ? autostart::Enable(selfPath_, err) : autostart::Disable(err)) {
            startWithWindows_ = on;
            Log(on ? "Start with Windows on - Project OptM starts in the tray when you sign in, as admin, without the Windows prompt"
                   : "Start with Windows off");
        } else Log(std::string("Couldn't turn Start with Windows ") + (on ? "on" : "off") + ": " + err);
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", testCopy ? "Not available in a test copy (--data-dir)"
                                         : "Starts Project OptM in the tray when you sign in to Windows - already running as\n"
                                           "administrator, so there's no \"allow this app to make changes?\" prompt.\n"
                                           "Uses a Task Scheduler task; turning this off removes it.");
    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(kSub, "Everything Project OptM changes is put back when the game closes, when you exit, or instantly with %s.%s",
                       hotkey_ ? "Ctrl+Alt+End" : "the tray menu",
                       data_.revertOnExit ? "" : " (Launch priority, GPU preference and fullscreen optimizations stay set after you exit.)");
    ImGui::TextColored(kDim, "Game checks run every %d s. Timing, power plan, background apps and more are set in the [Settings] block of your profiles file.", data_.settings.poll);
    ImGui::PopTextWrapPos();
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("appearance");
    Label("APPEARANCE");
    ImGui::Dummy(ImVec2(0, 4 * s_));
    ImGui::TextColored(kSub, "Accent color");
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < sizeof(kAccents) / sizeof(kAccents[0]); i++) {
        if (i) ImGui::SameLine(0, 8 * s_);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float sz = 30 * s_;
        ImGui::PushID((int)i);
        if (ImGui::InvisibleButton("sw", ImVec2(sz, sz))) {
            data_.theme.accent = kAccents[i].second;
            snprintf(hexBuf_, sizeof(hexBuf_), "%s", kAccents[i].second);
            ApplyTheme(); data_.SaveConfig();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", kAccents[i].first);
        ImGui::PopID();
        dl->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), U32(Hex(kAccents[i].second)), g_btnR);
        if (util::Lower(data_.theme.accent) == util::Lower(kAccents[i].second))
            dl->AddCircleFilled(ImVec2(p.x + sz / 2, p.y + sz / 2), 5 * s_, U32(TextOn(Hex(kAccents[i].second))), 16);
    }
    ImGui::SetNextItemWidth(160 * s_);
    ImGui::InputText("##hex", hexBuf_, sizeof(hexBuf_), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if (ImGui::Button("Use")) {
        std::string v = hexBuf_;
        if (!v.empty() && v[0] != '#') v = "#" + v;
        bool ok = v.size() == 7 && v.find_first_not_of("0123456789abcdefABCDEF", 1) == std::string::npos;
        if (ok) {
            for (auto& ch : v) ch = (char)toupper((unsigned char)ch);
            data_.theme.accent = v; ApplyTheme(); data_.SaveConfig();
        } else Log("Color '" + v + "' isn't valid - use #RRGGBB, like #FF6A00");
    }
    ImGui::Dummy(ImVec2(0, 4 * s_));
    ImGui::TextColored(kSub, "Background");
    for (size_t i = 0; i < 4; i++) {
        if (i) ImGui::SameLine();
        if (toggle(kBackgrounds[i].name, data_.theme.background == kBackgrounds[i].name)) { data_.theme.background = kBackgrounds[i].name; ApplyTheme(); data_.SaveConfig(); }
    }
    ImGui::TextColored(kSub, "Corners");
    for (size_t i = 0; i < 3; i++) {
        if (i) ImGui::SameLine();
        if (toggle(kCorners[i].name, data_.theme.corners == kCorners[i].name)) { data_.theme.corners = kCorners[i].name; ApplyTheme(); data_.SaveConfig(); }
    }
    ImGui::TextColored(kSub, "Interface size");
    {
        char autoLabel[32];
        float autoZoom = 0;
        { double keep = data_.uiScale; data_.uiScale = 0; autoZoom = Zoom(); data_.uiScale = keep; }
        snprintf(autoLabel, sizeof(autoLabel), "Auto (%d%%)", (int)std::lround(autoZoom * 100));
        if (toggle(autoLabel, data_.uiScale <= 0)) SetZoom(0);
        for (float z : kZooms) {
            char b[16]; snprintf(b, sizeof(b), "%d%%", (int)std::lround(z * 100));
            ImGui::SameLine();
            if (toggle(b, data_.uiScale > 0 && std::fabs(data_.uiScale - z) < 0.01)) SetZoom(z);
        }
        ImGui::TextColored(kDim, "Ctrl + and Ctrl - change it anywhere, Ctrl 0 goes back to Auto.");
    }
    ImGui::TextColored(kSub, "Animations");
    if (toggle(data_.animations ? "Animations: ON" : "Animations: OFF", data_.animations)) {
        data_.animations = !data_.animations;
        data_.SaveConfig();
    }
    ImGui::SameLine();
    ImGui::TextColored(kDim, "Page fades, sliding switches and highlights");
    ImGui::Dummy(ImVec2(0, 2 * s_));
    if (ImGui::Button("Reset look")) {
        data_.theme = Theme();
        snprintf(hexBuf_, sizeof(hexBuf_), "%s", data_.theme.accent.c_str());
        ApplyTheme(); data_.SaveConfig();
    }
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("data");
    Label("PROFILES AND DATA");
    ImGui::Dummy(ImVec2(0, 2 * s_));
    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(kSub, "Your games live in profiles.ini - it opens in Notepad, and saving reloads it instantly.");
    ImGui::PopTextWrapPos();
    if (ImGui::Button("Edit profiles")) OpenProfiles();
    ImGui::SameLine();
    if (ImGui::Button("Open data folder")) util::OpenAsUser(data_.DataDir());
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        data_.LoadConfig(); data_.LoadHistory(); ApplyTheme();
        ReloadProfiles();
        snprintf(hexBuf_, sizeof(hexBuf_), "%s", data_.theme.accent.c_str());
    }
    EndCard();
    ImGui::Dummy(ImVec2(0, 6 * s_));

    BeginCard("about");
    Label("ABOUT");
    ImGui::Dummy(ImVec2(0, 2 * s_));
    ImGui::PushFont(fontBold_);
    ImGui::TextUnformatted("Project OptM v" OPTM_VERSION);
    ImGui::PopFont();
    std::string st = !updater_.Enabled() ? "Updates are off (no GitHub repo set in version.h)."
                   : !updater_.Status().empty() ? updater_.Status() : std::string("Updates from github.com/") + OPTM_UPDATE_REPO;
    ImGui::TextColored(kSub, "%s", st.c_str());
    ImGui::BeginDisabled(!updater_.Enabled());
    if (ImGui::Button("Check for updates")) { lastUpdateCheck_ = Ms(); updater_.Check(true); }
    ImGui::SameLine();
    if (toggle(data_.autoUpdate ? "Auto-check: ON" : "Auto-check: OFF", data_.autoUpdate)) { data_.autoUpdate = !data_.autoUpdate; data_.SaveConfig(); }
    ImGui::EndDisabled();
    if (ImGui::Button("Show the tour again")) StartTour();
    ImGui::SameLine();
    ImGui::TextColored(kDim, "Ctrl + 1 to 6 switches tabs. Ctrl + and Ctrl - change the interface size.");
    EndCard();
    ImGui::EndChild();
}


// ============================================================ Tweaks page
namespace {
ImVec4 PresetColor(const std::string& name) {
    if (name == "Safe") return Hex("#3DDC84");
    if (name == "Balanced") return Hex("#6FA0FF");
    if (name == "Aggressive") return Hex("#F0605D");
    return kText;
}
unsigned CategoryIcon(const std::string& c) {
    if (c == "CPU") return 0xE950;
    if (c == "Memory") return 0xE964;
    if (c == "System") return 0xE7F4;
    if (c == "GPU") return 0xE945;
    return 0xE83E;   // Power
}
bool Matches(const tweakset::Tweak& t, const std::string& q) {
    if (q.empty()) return true;
    return util::Lower(t.name).find(q) != std::string::npos || util::Lower(t.desc).find(q) != std::string::npos ||
           util::Lower(t.category).find(q) != std::string::npos;
}
}  // namespace

// A toggle switch: light track + dark knob when on
bool App::Switch(const char* id, bool on, bool enabled) {
    ImVec2 size(44 * s_, 24 * s_);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("sw", size) && enabled;
    bool hov = ImGui::IsItemHovered() && enabled;
    ImGui::PopID();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float a = enabled ? 1.0f : 0.35f;
    float k = Anim(std::string("sw.") + id, on ? 1.0f : 0.0f, 16.0f);   // 0 = off, 1 = on; the knob slides
    auto mix = [](ImVec4 x, ImVec4 y, float t) { return ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t); };
    ImVec4 offTrack = hov ? Lighten(g_chip, 0.08f) : Lighten(g_chip, 0.03f);
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), U32(Alpha(mix(offTrack, kText, k), a)), size.y / 2);
    float r = size.y / 2 - 3 * s_;
    ImVec2 c(p.x + size.y / 2 + (size.x - size.y) * k, p.y + size.y / 2);
    dl->AddCircleFilled(c, r, U32(Alpha(mix(kSub, Hex("#0B0C0F"), k), a)), 24);
    return clicked;
}

std::string App::UniquePresetName(const std::string& base) const {
    std::string name = base;
    for (int i = 2; tweakset::IsBuiltIn(name) || data_.tweakPresets.count(name) || name == "Custom"; i++) name = base + " " + std::to_string(i);
    return name;
}

void App::TweaksChanged() {
    data_.SaveConfig();
    opt_.RefreshTweaks();
    opt_.SyncPerGameSettings();
}

void App::SelectPreset(const std::string& name) {
    if (data_.tweakPreset == name) return;
    data_.tweakPreset = name;
    renaming_ = false;
    TweaksChanged();
    Log("Tweaks preset: " + name + (opt_.Active() ? " (applies to the next game)" : ""));
}

void App::SetTweak(const std::string& id, bool on) {
    auto set = tweakset::PresetTweaks(data_, data_.tweakPreset);
    if (on) set.insert(id); else set.erase(id);
    if (tweakset::IsBuiltIn(data_.tweakPreset)) {   // built-ins are locked: edit a copy
        std::string name = UniquePresetName("My tweaks");
        Log("Made an editable copy of " + data_.tweakPreset + ": " + name);
        data_.tweakPreset = name;
    }
    data_.tweakPresets[data_.tweakPreset] = std::vector<std::string>(set.begin(), set.end());
    TweaksChanged();
}

void App::ExportPreset() {
    std::wstring file = util::Widen(data_.tweakPreset) + L".optm-tweaks.json";
    for (auto& ch : file) if (wcschr(L"\\/:*?\"<>|", ch)) ch = L'_';
    wchar_t buf[MAX_PATH * 2] = {};
    wcsncpy_s(buf, file.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Project OptM tweaks (*.json)\0*.json\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH * 2;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return;
    if (util::WriteFile(buf, tweakset::ExportJson(data_.tweakPreset, tweakset::PresetTweaks(data_, data_.tweakPreset))))
        Log("Exported tweaks preset '" + data_.tweakPreset + "' to " + util::Narrow(buf));
    else Log("Couldn't save " + util::Narrow(buf));
}

void App::ImportPreset() {
    wchar_t buf[MAX_PATH * 2] = {};
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Project OptM tweaks (*.json)\0*.json\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH * 2;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;
    std::string text, name;
    std::set<std::string> on;
    if (!util::ReadFile(buf, text) || !tweakset::ImportJson(text, name, on)) { Log("That file isn't a Project OptM tweaks preset"); return; }
    name = UniquePresetName(util::Trim(name).empty() ? "Imported" : util::Trim(name));
    data_.tweakPresets[name] = std::vector<std::string>(on.begin(), on.end());
    data_.tweakPreset = name;
    TweaksChanged();
    Log("Imported tweaks preset '" + name + "' (" + std::to_string(on.size()) + " tweaks on)");
}

void App::PageTweaks() {
    const auto& all = tweakset::All();
    auto chosen = tweakset::PresetTweaks(data_, data_.tweakPreset);
    auto active = tweakset::Active(data_, sys_);
    const bool builtIn = tweakset::IsBuiltIn(data_.tweakPreset);

    // ---- header: summary + Import + search
    float searchW = 320 * s_;
    float importW = ImGui::CalcTextSize("Import").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::AlignTextToFramePadding();
    size_t own = std::count_if(data_.profiles.begin(), data_.profiles.end(), [](const GameProfile& p) { return !p.tweaks.empty(); });
    ImGui::TextColored(kSub, "%zu of %zu tweaks on in %s. Changes apply to the next game.%s", active.size(), all.size(), data_.tweakPreset.c_str(),
                       own ? ("  " + Plural(own, "game") + " set their own.").c_str() : "");
    if (own && ImGui::IsItemHovered()) ImGui::SetTooltip("Those games use the tweaks picked with the gear on their tile (Games page)");
    ImGui::SameLine(ImGui::GetContentRegionMax().x - searchW - importW - 8 * s_);
    if (ImGui::Button("Import")) ImportPreset();
    ImGui::SameLine(0, 8 * s_);
    ImGui::SetNextItemWidth(searchW);
    if (focusSearch_) { ImGui::SetKeyboardFocusHere(); focusSearch_ = false; }
    ImGui::InputTextWithHint("##search", "Search tweaks  (Ctrl+F)", tweakSearch_, sizeof(tweakSearch_));
    std::string query = util::Lower(util::Trim(tweakSearch_));
    ImGui::Dummy(ImVec2(0, 4 * s_));

    // ---- preset cards
    ImVec2 presetsTop = ImGui::GetCursorScreenPos();
    std::vector<std::string> presets = tweakset::BuiltIns();
    for (auto& [name, ids] : data_.tweakPresets) presets.push_back(name);
    float avail = ImGui::GetContentRegionAvail().x, gap = 12 * s_;
    int perRow = std::max(2, std::min((int)presets.size() + 1, (int)(avail / (190 * s_))));
    float cardW = (avail - gap * (perRow - 1)) / perRow, cardH = 76 * s_;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (size_t i = 0; i <= presets.size(); i++) {
        if (i % perRow) ImGui::SameLine(0, gap);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushID((int)i);
        bool clicked = ImGui::InvisibleButton("preset", ImVec2(cardW, cardH));
        bool hov = ImGui::IsItemHovered();
        ImGui::PopID();
        ImVec2 q(p.x + cardW, p.y + cardH);
        dl->AddRectFilled(p, q, U32(hov ? Lighten(g_card, 0.03f) : g_card), g_cardR);
        if (i == presets.size()) {   // "+ New"
            const char* t = "+   New";
            ImVec2 ts = ImGui::CalcTextSize(t);
            dl->AddRect(p, q, U32(g_line), g_cardR);
            dl->AddText(ImVec2(p.x + (cardW - ts.x) / 2, p.y + (cardH - ts.y) / 2), U32(kSub), t);
            if (clicked) {
                std::string name = UniquePresetName("My tweaks");
                data_.tweakPresets[name] = std::vector<std::string>(chosen.begin(), chosen.end());
                data_.tweakPreset = name;
                TweaksChanged();
                renaming_ = true;
                snprintf(renameBuf_, sizeof(renameBuf_), "%s", name.c_str());
                Log("New tweaks preset: " + name + " (a copy of your current tweaks)");
            }
            continue;
        }
        const std::string& name = presets[i];
        bool sel = name == data_.tweakPreset;
        if (sel) dl->AddRect(p, q, U32(Alpha(kText, 0.85f)), g_cardR, 0, 1.5f * s_);
        dl->AddCircleFilled(ImVec2(p.x + 22 * s_, p.y + 26 * s_), 5 * s_, U32(PresetColor(name)), 16);
        dl->PushClipRect(p, ImVec2(q.x - 30 * s_, q.y), true);
        dl->AddText(fontBold_, fontBold_->FontSize, ImVec2(p.x + 36 * s_, p.y + 26 * s_ - fontBold_->FontSize / 2), U32(kText), name.c_str());
        dl->PopClipRect();
        std::string ic = Icon(tweakset::IsBuiltIn(name) ? 0xE72E : 0xE70F);   // lock / pencil
        dl->AddText(fontIcons_, fontIcons_->FontSize * 0.8f, ImVec2(q.x - 30 * s_, p.y + 26 * s_ - fontIcons_->FontSize * 0.4f), U32(kDim), ic.c_str());
        size_t n = 0;
        for (auto& id : tweakset::PresetTweaks(data_, name)) if (tweakset::Unavailable(id, sys_, data_).empty()) n++;
        char b[48]; snprintf(b, sizeof(b), "%zu of %zu on", n, all.size());
        dl->AddText(ImVec2(p.x + 22 * s_, p.y + 48 * s_), U32(kDim), b);
        if (clicked) SelectPreset(name);
    }
    ImGui::Dummy(ImVec2(0, 4 * s_));

    // ---- selected preset bar
    BeginCard("presetbar");
    ImGui::AlignTextToFramePadding();
    ImGui::PushFont(fontIcons_);
    ImGui::TextColored(kSub, "%s", Icon(builtIn ? 0xE72E : 0xE70F).c_str());
    ImGui::PopFont();
    ImGui::SameLine(0, 12 * s_);
    if (renaming_ && !builtIn) {
        ImGui::SetNextItemWidth(220 * s_);
        if (ImGui::IsWindowAppearing() || renameBuf_[0] == 0) ImGui::SetKeyboardFocusHere();
        bool enter = ImGui::InputText("##rename", renameBuf_, sizeof(renameBuf_), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::SameLine();
        if (enter || ImGui::Button("Save")) {
            std::string nn = util::Trim(renameBuf_);
            if (nn.empty() || nn == data_.tweakPreset) renaming_ = false;
            else if (tweakset::IsBuiltIn(nn) || data_.tweakPresets.count(nn)) Log("There's already a preset called '" + nn + "'");
            else if (nn == "Custom" || util::Lower(nn) == "default") Log("'" + nn + "' is reserved for games' own tweaks - pick another name");
            else {
                data_.tweakPresets[nn] = data_.tweakPresets[data_.tweakPreset];
                data_.tweakPresets.erase(data_.tweakPreset);
                if (data_.ProfilesChanged()) ReloadProfiles();
                for (auto p : data_.profiles)   // games set to this preset follow the new name
                    if (p.tweaks == data_.tweakPreset) { p.tweaks = nn; data_.SaveProfile(p); }
                data_.LoadProfiles();
                Log("Renamed tweaks preset to '" + nn + "'");
                data_.tweakPreset = nn;
                renaming_ = false;
                TweaksChanged();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) renaming_ = false;
    } else {
        ImGui::PushFont(fontBold_);
        ImGui::TextUnformatted(data_.tweakPreset.c_str());
        ImGui::PopFont();
        ImGui::SameLine(0, 12 * s_);
        ImGui::TextColored(kDim, "%s", builtIn ? "Built-in preset. Flip any switch to make your own copy." : "Your preset. Changes save as you make them.");
    }
    {
        float bw = 0;
        const char* labels[] = { "Rename", "Duplicate", "Export", "Delete" };
        for (auto* l : labels) bw += ImGui::CalcTextSize(l).x + ImGui::GetStyle().FramePadding.x * 2 + 8 * s_;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - bw + 8 * s_);
        ImGui::BeginDisabled(builtIn || renaming_);
        if (ImGui::Button("Rename")) { renaming_ = true; snprintf(renameBuf_, sizeof(renameBuf_), "%s", data_.tweakPreset.c_str()); }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 8 * s_);
        if (ImGui::Button("Duplicate")) {
            std::string name = UniquePresetName(data_.tweakPreset + " copy");
            data_.tweakPresets[name] = std::vector<std::string>(chosen.begin(), chosen.end());
            SelectPreset(name);
        }
        ImGui::SameLine(0, 8 * s_);
        if (ImGui::Button("Export")) ExportPreset();
        ImGui::SameLine(0, 8 * s_);
        ImGui::BeginDisabled(builtIn);
        if (ImGui::Button("Delete") &&
            Msg(hwnd_, "Delete the tweaks preset '" + data_.tweakPreset + "'?", "Project OptM", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            std::string gone = data_.tweakPreset;
            data_.tweakPresets.erase(gone);
            data_.tweakPreset = "Safe";
            renaming_ = false;
            TweaksChanged();
            Log("Deleted tweaks preset '" + gone + "' - now using Safe");
        }
        ImGui::EndDisabled();
    }
    EndCard();
    marks_["presets"] = ImVec4(presetsTop.x, presetsTop.y, ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y);
    ImGui::Dummy(ImVec2(0, 4 * s_));

    // ---- category chips
    std::vector<std::string> cats = { "All" };
    for (auto* c : tweakset::Categories()) cats.push_back(c);
    for (size_t i = 0; i < cats.size(); i++) {
        size_t n = 0;
        for (auto& t : all) if (cats[i] == "All" || cats[i] == t.category) n++;
        char b[48]; snprintf(b, sizeof(b), "%s  %zu", cats[i].c_str(), n);
        if (i) ImGui::SameLine(0, 8 * s_);
        bool sel = tweakCat_ == cats[i];
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, sel ? Lighten(g_chip, 0.08f) : g_card);
        ImGui::PushStyleColor(ImGuiCol_Text, sel ? kText : kSub);
        if (ImGui::Button(b)) tweakCat_ = cats[i];
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
    }
    {   // right side: expand every tweak's pros and cons at once
        const char* label = "Show all details";
        float lw = ImGui::CalcTextSize(label).x;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - lw - 44 * s_ - 10 * s_);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kSub, "%s", label);
        ImGui::SameLine(0, 10 * s_);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - 24 * s_) / 2);
        if (Switch("allDetails", showAllDetails_, true)) showAllDetails_ = !showAllDetails_;
    }
    ImGui::Dummy(ImVec2(0, 6 * s_));

    // ---- cards: one per category, two columns
    auto card = [&](const char* cat) {
        std::vector<const tweakset::Tweak*> rows;
        size_t total = 0, onCount = 0;
        for (auto& t : all) {
            if (std::string(t.category) != cat) continue;
            total++;
            if (active.count(t.id)) onCount++;
            if (Matches(t, query)) rows.push_back(&t);
        }
        if (rows.empty()) return;
        BeginCard(cat);
        // header
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(fontIcons_);
        ImGui::TextColored(kSub, "%s", Icon(CategoryIcon(cat)).c_str());
        ImGui::PopFont();
        ImGui::SameLine(0, 12 * s_);
        ImGui::PushFont(fontBold_);
        ImGui::TextUnformatted(cat);
        ImGui::PopFont();
        char cnt[32]; snprintf(cnt, sizeof(cnt), "%zu / %zu", onCount, total);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(cnt).x);
        ImGui::TextColored(kDim, "%s", cnt);
        ImDrawList* d = ImGui::GetWindowDrawList();
        for (auto* t : rows) {
            ImVec2 lp = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            d->AddLine(ImVec2(lp.x, lp.y + 2 * s_), ImVec2(lp.x + w, lp.y + 2 * s_), U32(g_line), 1.0f);
            ImGui::Dummy(ImVec2(0, 8 * s_));
            std::string why = tweakset::Unavailable(t->id, sys_, data_);
            bool avail2 = why.empty();
            bool isOn = chosen.count(t->id) && avail2;
            float rowY = ImGui::GetCursorPosY();
            ImGui::BeginGroup();
            ImGui::PushFont(fontBold_);
            ImGui::TextColored(avail2 ? kText : kDim, "%s", t->name);
            ImGui::PopFont();
            // badge: availability reason, or the tweak's own warning
            std::string badge = !why.empty() ? why : t->badge;
            if (!badge.empty()) {
                ImGui::SameLine(0, 10 * s_);
                ImGui::SetWindowFontScale(0.85f);
                ImVec2 ts = ImGui::CalcTextSize(badge.c_str());
                ImVec2 bp = ImGui::GetCursorScreenPos();
                bool warn = why.empty();
                ImVec2 pad(8 * s_, 2 * s_);
                d->AddRectFilled(ImVec2(bp.x, bp.y), ImVec2(bp.x + ts.x + pad.x * 2, bp.y + ts.y + pad.y * 2),
                                 U32(warn ? Alpha(kAmber, 0.14f) : g_chip), 6 * s_);
                d->AddText(ImVec2(bp.x + pad.x, bp.y + pad.y), U32(warn ? kAmber : kSub), badge.c_str());
                ImGui::Dummy(ImVec2(ts.x + pad.x * 2, ts.y + pad.y * 2));
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - 100 * s_);
            ImGui::TextColored(kDim, "%s", t->id == std::string("pinning") && sys_.canPin
                                             ? ("Pins the game to your " + sys_.BestLabel() + " (CPU " + SystemInfo::MaskText(sys_.bestMask) + ")").c_str()
                                             : t->desc);
            ImGui::PopTextWrapPos();
            ImGui::EndGroup();
            // click the text to show what it does, its pros and its cons
            bool open = showAllDetails_ || expanded_.count(t->id);
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked() && !showAllDetails_) { if (open) expanded_.erase(t->id); else expanded_.insert(t->id); open = !open; }
            float rowH = ImGui::GetCursorPosY() - rowY;
            ImVec2 after = ImGui::GetCursorPos();
            float right = ImGui::GetWindowContentRegionMax().x;
            std::string chev = Icon(open ? 0xE70E : 0xE70D);   // chevron up / down
            float chevSize = fontIcons_->FontSize * 0.7f;
            d->AddText(fontIcons_, chevSize, ImVec2(ImGui::GetWindowPos().x + right - 44 * s_ - 26 * s_ - ImGui::GetScrollX(),
                                                    ImGui::GetWindowPos().y + rowY - ImGui::GetScrollY() + (rowH - chevSize) / 2), U32(kDim), chev.c_str());
            ImGui::SetCursorPos(ImVec2(right - 44 * s_, rowY + (rowH - 24 * s_) / 2));
            if (Switch(t->id, isOn, avail2)) SetTweak(t->id, !isOn);
            if (ImGui::IsItemHovered() && !avail2) ImGui::SetTooltip("%s", why.c_str());
            ImGui::SetCursorPos(after);
            std::string detKey = std::string("det.") + t->id;
            if (!open) anim_.erase(detKey);   // fades in again next time it opens
            if (open) {
                const tweakset::Details& det = tweakset::DetailsOf(t->id);
                float fa = Anim(detKey, 1.0f, 12.0f, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * fa);
                ImGui::Dummy(ImVec2(0, 2 * s_));
                ImVec2 bp = ImGui::GetCursorScreenPos();
                d->ChannelsSplit(2);
                d->ChannelsSetCurrent(1);
                ImGui::Indent(12 * s_);
                ImGui::Dummy(ImVec2(0, 4 * s_));
                float wrap = ImGui::GetCursorPosX() + w - 36 * s_;
                auto section = [&](const char* label, const char* text, ImVec4 col) {
                    ImGui::PushFont(fontBold_);
                    ImGui::TextColored(col, "%s", label);
                    ImGui::PopFont();
                    ImGui::PushTextWrapPos(wrap);
                    ImGui::TextColored(kSub, "%s", text);
                    ImGui::PopTextWrapPos();
                    ImGui::Dummy(ImVec2(0, 2 * s_));
                };
                section("What it does", det.what, kText);
                section("Pros", det.pros, kGreen);
                section("Cons", det.cons, kAmber);
                ImGui::Unindent(12 * s_);
                ImVec2 be = ImVec2(bp.x + w - 12 * s_, ImGui::GetCursorScreenPos().y);
                d->ChannelsSetCurrent(0);
                d->AddRectFilled(bp, be, U32(Alpha(g_card2, fa)), g_btnR);
                d->ChannelsMerge();
                ImGui::PopStyleVar();
            }
            ImGui::Dummy(ImVec2(0, 4 * s_));
        }
        EndCard();
        ImGui::Dummy(ImVec2(0, 6 * s_));
    };

    if (tweakCat_ != "All") { card(tweakCat_.c_str()); return; }
    float colW = (ImGui::GetContentRegionAvail().x - 14 * s_) / 2;
    ImGui::BeginChild("tweaksL", ImVec2(colW, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    card("CPU");
    card("Power");
    ImGui::EndChild();
    ImGui::SameLine(0, 14 * s_);
    ImGui::BeginChild("tweaksR", ImVec2(colW, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
    card("Memory");
    card("System");
    card("GPU");
    ImGui::EndChild();
}