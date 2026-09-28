#include "screens.h"
#include "util.h"
#include <set>
#include <dwmapi.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

namespace screens {

namespace {
int Hz(HMONITOR m) {
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(m, &mi)) return 0;
    DEVMODEW dm = { };
    dm.dmSize = sizeof(dm);
    return EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1 ? (int)dm.dmDisplayFrequency : 0;
}

// apps that play videos and streams (browsers, players) - voice chat and music apps aren't counted
const char* kMediaApps[] = {
    "chrome", "msedge", "firefox", "opera", "opera_gx", "brave", "vivaldi", "arc", "vlc", "mpc-hc", "mpc-hc64", "mpc-be", "mpc-be64",
    "potplayer", "potplayermini", "potplayermini64", "mpv", "wmplayer", "microsoft.media.player", "video.ui", "plex", "plex htpc", "netflix",
};

bool IsMediaApp(const std::string& lower) {
    for (auto* n : kMediaApps) if (lower == n) return true;
    return false;
}

// process id -> exe name without .exe, lower-case
std::string NameOf(DWORD pid) {
    std::wstring p;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) {
        wchar_t buf[MAX_PATH]; DWORD n = MAX_PATH;
        if (QueryFullProcessImageNameW(h, 0, buf, &n)) p.assign(buf, n);
        CloseHandle(h);
    }
    if (p.empty()) return "";
    p = p.substr(p.find_last_of(L"\\/") + 1);
    return util::Lower(util::StripExe(util::Narrow(p)));
}

// lower-case names of apps that are making sound right now (on any output)
std::set<std::string> Playing() {
    std::set<std::string> out;
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IMMDeviceEnumerator* en = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en))) {
        IMMDeviceCollection* devs = nullptr;
        if (SUCCEEDED(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devs))) {
            UINT n = 0;
            devs->GetCount(&n);
            for (UINT i = 0; i < n; i++) {
                IMMDevice* d = nullptr;
                IAudioSessionManager2* mgr = nullptr;
                IAudioSessionEnumerator* se = nullptr;
                if (SUCCEEDED(devs->Item(i, &d)) && SUCCEEDED(d->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&mgr)) &&
                    SUCCEEDED(mgr->GetSessionEnumerator(&se))) {
                    int c = 0;
                    se->GetCount(&c);
                    for (int j = 0; j < c; j++) {
                        IAudioSessionControl* ctl = nullptr;
                        IAudioSessionControl2* ctl2 = nullptr;
                        IAudioMeterInformation* meter = nullptr;
                        if (SUCCEEDED(se->GetSession(j, &ctl)) && SUCCEEDED(ctl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&ctl2)) &&
                            SUCCEEDED(ctl->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&meter))) {
                            DWORD pid = 0;
                            float peak = 0;
                            if (SUCCEEDED(ctl2->GetProcessId(&pid)) && pid && SUCCEEDED(meter->GetPeakValue(&peak)) && peak > 0.001f) {
                                std::string nm = NameOf(pid);
                                if (!nm.empty()) out.insert(nm);
                            }
                        }
                        if (meter) meter->Release();
                        if (ctl2) ctl2->Release();
                        if (ctl) ctl->Release();
                    }
                }
                if (se) se->Release();
                if (mgr) mgr->Release();
                if (d) d->Release();
            }
            devs->Release();
        }
        en->Release();
    }
    if (init == S_OK || init == S_FALSE) CoUninitialize();
    return out;
}
}  // namespace

std::vector<Screen> All() {
    std::vector<Screen> v;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR m, HDC, LPRECT, LPARAM lp) -> BOOL {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(m, &mi);
        ((std::vector<Screen>*)lp)->push_back({ m, Hz(m), mi.rcMonitor, (mi.dwFlags & MONITORINFOF_PRIMARY) != 0 });
        return TRUE;
    }, (LPARAM)&v);
    return v;
}

int RefreshOf(HWND w) { return w ? Hz(MonitorFromWindow(w, MONITOR_DEFAULTTONEAREST)) : 0; }

bool MediaOnOtherScreen(HWND game, std::string& app, int& hz) {
    if (!game) return false;
    std::set<std::string> media;
    for (auto& n : Playing()) if (IsMediaApp(n)) media.insert(n);
    if (media.empty()) return false;
    struct Ctx { HMONITOR gameMon; const std::set<std::string>* media; std::string app; HMONITOR mon = nullptr; } c;
    c.gameMon = MonitorFromWindow(game, MONITOR_DEFAULTTONEAREST);
    c.media = &media;
    // a visible, not minimized, not hidden-by-Windows window of that app on another screen
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* c = (Ctx*)lp;
        if (!IsWindowVisible(h) || IsIconic(h) || GetWindow(h, GW_OWNER)) return TRUE;
        BOOL cloaked = FALSE;
        DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        if (cloaked) return TRUE;
        RECT r;
        if (!GetWindowRect(h, &r) || r.right - r.left < 320 || r.bottom - r.top < 180) return TRUE;
        HMONITOR m = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
        if (m == c->gameMon) return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        std::string n = NameOf(pid);
        if (!c->media->count(n)) return TRUE;
        c->app = n;
        c->mon = m;
        return FALSE;
    }, (LPARAM)&c);
    if (c.app.empty()) return false;
    app = c.app;
    hz = Hz(c.mon);
    return true;
}

}  // namespace screens
