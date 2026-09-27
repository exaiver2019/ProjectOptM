// Project OptM - the app window: sidebar, pages, theme, tray icon.
#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include "checks.h"
#include "data.h"
#include "detect.h"
#include "frames.h"
#include "imgui.h"
#include "optimizer.h"
#include "overlay.h"
#include "sensors.h"
#include "processes.h"
#include "system_info.h"
#include "updater.h"

constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr int kHotkeyPanic = 0x4F50;
constexpr int kHotkeyOverlay = 0x4F51;   // Ctrl+Alt+O

class App {
public:
    void Init(HWND hwnd, float dpiScale);
    void Update();                 // background work; runs every loop, even while hidden
    void Render();                 // draws one frame
    void Shutdown();               // puts everything back before exit
    bool Busy() const;             // true while something animates / is being dragged
    DWORD MaxWaitMs() const;       // longest the main loop may sleep (the overlay needs regular redraws)
    bool WantsRestart() const { return restart_; }
    bool RestartHidden() const { return restartHidden_; }   // restart into the tray

    // interface size = Windows' display scaling x the user's size setting
    float Scale() const { return s_; }
    bool NeedsRescale() const { return rescale_; }
    void Rescale();                          // call between frames (rebuilds fonts)
    void OnDpiChanged(float dpi);
    SIZE WindowSize(const RECT& work) const; // starting window size

    // window / tray events (from main.cpp)
    void OnTray(LPARAM lp);
    void OnCommand(WPARAM id);
    void OnHotkey();
    void ToggleOverlay();          // Ctrl+Alt+O
    void OnActivate();
    void OnMinimize();
    void OnTaskbarCreated();
    void ShowMain();

private:
    enum Page { Home, Games, Sessions, Overlay, Tweaks, System, Activity, Settings, About, PageCount };

    // setup
    void LoadFonts();
    void ApplyTheme();
    float Zoom() const;                      // user setting, or automatic for big screens
    void SetZoom(double z);                  // 0 = automatic
    void Log(const std::string& line);

    // background work
    void Tick();
    void ReloadProfiles();
    void RefreshChecks();
    void AddDetectedGame(const DetectedGame& g);
    void NotAGame(const std::string& name);
    void SessionStarted(const GameProfile& p);
    void SessionEnded(const GameProfile& p, double minutes, bool gameClosed);
    void UpdateTray();
    void UpdateOverlay();          // show / refresh / hide the in-game overlay
    OverlayContent OverlayNow();   // what it shows right now
    float OverlayScale() const;
    int OverlayIntervalMs() const;
    void OverlayPositioner();      // the Overlay page's picture of your screen: drag the overlay into place
    HWND GameWindow();             // the running game's main window (cached)
    void Balloon(const char* text, DWORD icon = NIIF_INFO);

    // actions
    void ToggleAuto();
    void Panic();
    bool PickLauncher(const std::string& game);
    void StartGame(const std::string& game);
    void ShowPlan(const GameProfile& p);
    void ClearShaderCache();
    void OpenProfiles();

    // layout pieces
    void Sidebar(float height);
    void StatusBar();
    void PageHome();
    void PageGames();
    void PageSessions();
    void PageOverlay();
    void PageAbout();
    void HowItWorksPopup();
    void PageSystem();
    void PageActivity();
    void PageSettings();
    void PageTweaks();
    bool Switch(const char* id, bool on, bool enabled);
    void SetTweak(const std::string& id, bool on);
    void SelectPreset(const std::string& name);
    void TweaksChanged();
    std::string UniquePresetName(const std::string& base) const;
    void ExportPreset();
    void ImportPreset();
    void PerfCard();
    void HistoryPopup();
    void OpenGameSettings(const std::string& name);
    void GameSettingsPopup();
    bool SaveGameSettings();       // false (and says why) if something needs fixing first
    void DeleteGame(const std::string& name);
    void OpenFeedback();
    void FeedbackPopup();
    std::string FeedbackTitle() const;
    std::string FeedbackBody() const;              // the issue text, personal paths hidden
    void SendFeedback();                           // opens a filled-in GitHub issue

    // animation + tour
    float Anim(const std::string& key, float target, float speed = 14.0f, float init = -1.0f);   // eases toward target
    void Mark(const char* key);                     // remembers the last item's rect (tour spotlight)
    void StartTour();
    void TourGo(int step);
    void TourOverlay();
    void IntroOverlay();                            // the animated greeting when the app opens

    // widgets
    bool BeginCard(const char* id, float height = 0);
    void EndCard();
    void Label(const char* text);
    void Logo(float size);
    void VendorBadge(const std::string& vendor);
    bool AccentButton(const char* label, const ImVec2& size);
    void Chips();
    std::vector<std::pair<std::string, std::string>> HardwareLines() const;
    std::vector<std::pair<const char*, std::string>> SpecRows() const;

    // window + scale
    HWND hwnd_ = nullptr;
    float s_ = 1.0f;               // total UI scale (dpi_ x zoom)
    float dpi_ = 1.0f;             // Windows display scaling of the window's monitor
    bool rescale_ = false;
    Page page_ = Home;

    // the engine and what it knows
    SystemInfo sys_;
    AppData data_;
    Optimizer opt_;
    ProcessList procs_;
    FrameCapture frames_;
    GameDetector detector_;
    Updater updater_;
    std::vector<Check> checks_;
    std::vector<std::string> log_;
    std::set<std::string> runningGames_;
    uint64_t startMs_ = 0, lastTick_ = 0, lastChecks_ = 0, lastUpdateCheck_ = 0;
    std::string lastCompare_;      // "+4 avg vs last session" for the session that just ended

    // self-restart when a new build replaces our exe
    std::wstring selfPath_;        // captured at start - after a rename Windows reports the new name
    int64_t selfStamp_ = 0;        // its file time at start
    uint64_t lastSelfCheck_ = 0;

    // Games page, game history and game settings
    char gameSearch_[64] = {};
    bool focusGameSearch_ = false;
    std::string historyGame_;      // game whose history popup is open
    std::string editGame_;         // game whose settings are open ("" = none)
    GameProfile edit_;             // its settings while being edited
    bool editDirty_ = false;
    std::string editError_;
    char exeBuf_[256] = {}, boostBuf_[256] = {}, keepBuf_[256] = {}, closeBuf_[256] = {};

    // Sessions page
    char sessionSearch_[64] = {};

    // Tweaks page
    char tweakSearch_[64] = {};
    std::string tweakCat_ = "All";
    bool renaming_ = false, focusSearch_ = false, showAllDetails_ = false;
    char renameBuf_[48] = {};      // preset being renamed
    std::set<std::string> expanded_;   // tweaks showing their pros and cons

    // Settings page
    char hexBuf_[16] = {};         // custom accent color
    char nameBuf_[40] = {};        // Your name (the intro's greeting)

    // About page
    bool howItWorks_ = false;      // its "How it works" window is open

    // feedback (a filled-in GitHub issue)
    bool feedbackOpen_ = false;
    int fbType_ = 0;               // Bug / Idea / Game request / Other
    char fbTitle_[120] = {};
    char fbDetails_[4096] = {};
    std::string fbGame_;           // "" = not about a game
    bool fbSpecs_ = true, fbLog_ = true, fbGameSettings_ = true, fbFps_ = true, fbPreview_ = false;
    std::string fbStatus_;

    // in-game overlay
    ::Overlay overlay_;            // (:: - "Overlay" is also a page name in here)
    Sensors sensors_;              // GPU / VRAM / CPU / RAM readings for it
    HWND gameWnd_ = nullptr;
    uint64_t lastOverlay_ = 0, lastGameWnd_ = 0;
    bool overlayHotkey_ = false;
    bool overlayDemo_ = false;     // --overlay-demo: show it over our own window (developer check)
    ImVec2 overlayGrab_;           // where in the overlay the positioner drag started

    // tray + lifetime
    NOTIFYICONDATAW nid_ = {};
    std::string trayTip_;
    bool trayAdded_ = false, trayTipShown_ = false, hotkey_ = false, restart_ = false, restartHidden_ = false, shutdown_ = false;
    bool startWithWindows_ = false; // Settings > Start with Windows (the Task Scheduler task exists)

    // animation + tour
    std::map<std::string, float> anim_;
    bool animNext_ = false, animating_ = false;   // an animation is still moving (keep drawing frames)
    int lastPage_ = -1;
    std::map<std::string, ImVec4> marks_;         // screen rects of tour targets (x0, y0, x1, y1)
    int tourStep_ = -1;                            // -1 = no tour
    bool introPending_ = false;                    // play the intro on the first frame
    uint64_t introStart_ = 0;                      // when it started (0 = not playing)
    bool introSkip_ = false;
    std::string introGreeting_, introLine_;

    // developer switches
    bool selfTest_ = false;        // --fps-self: graph our own frames
    double testZoom_ = 0;          // --zoom-after: change the interface size 1 s in
    float testScroll_ = 0;         // --scroll: screenshots of long pages

    ImFont* fontRegular_ = nullptr;
    ImFont* fontBold_ = nullptr;
    ImFont* fontTitle_ = nullptr;
    ImFont* fontBig_ = nullptr;
    ImFont* fontIcons_ = nullptr;
};
