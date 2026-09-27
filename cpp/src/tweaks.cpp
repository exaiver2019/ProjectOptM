#include "tweaks.h"
#include "util.h"
#include <cstdio>
#include <windows.h>
#include <powrprof.h>

namespace tweaks {

// ------------------------------------------------------------ services
namespace {
struct Scm {
    SC_HANDLE h;
    explicit Scm(DWORD access) : h(OpenSCManagerW(nullptr, nullptr, access)) {}
    ~Scm() { if (h) CloseServiceHandle(h); }
};
struct Svc {
    SC_HANDLE h = nullptr;
    Svc(const Scm& m, const std::string& name, DWORD access) { if (m.h) h = OpenServiceW(m.h, util::Widen(name).c_str(), access); }
    ~Svc() { if (h) CloseServiceHandle(h); }
};
}  // namespace

bool ServiceRunning(const std::string& name) {
    Scm m(SC_MANAGER_CONNECT);
    Svc s(m, name, SERVICE_QUERY_STATUS);
    SERVICE_STATUS st;
    return s.h && QueryServiceStatus(s.h, &st) && st.dwCurrentState == SERVICE_RUNNING;
}

bool StopService(const std::string& name) {
    Scm m(SC_MANAGER_CONNECT);
    Svc s(m, name, SERVICE_STOP | SERVICE_QUERY_STATUS);
    SERVICE_STATUS st;
    return s.h && ControlService(s.h, SERVICE_CONTROL_STOP, &st);
}

bool StartService(const std::string& name) {
    Scm m(SC_MANAGER_CONNECT);
    Svc s(m, name, SERVICE_START);
    return s.h && (StartServiceW(s.h, 0, nullptr) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);
}

bool ServiceExists(const std::string& pattern) {
    Scm m(SC_MANAGER_ENUMERATE_SERVICE);
    if (!m.h) return false;
    std::string pat = util::Lower(pattern);
    DWORD need = 0, count = 0, resume = 0;
    EnumServicesStatusExW(m.h, SC_ENUM_PROCESS_INFO, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, nullptr, 0, &need, &count, &resume, nullptr);
    std::vector<BYTE> buf(need + 4096);
    resume = 0;
    if (!EnumServicesStatusExW(m.h, SC_ENUM_PROCESS_INFO, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, buf.data(), (DWORD)buf.size(), &need, &count, &resume, nullptr))
        return false;
    auto* e = (ENUM_SERVICE_STATUS_PROCESSW*)buf.data();
    for (DWORD i = 0; i < count; i++) {
        std::string n = util::Lower(util::Narrow(e[i].lpServiceName));
        std::string d = util::Lower(util::Narrow(e[i].lpDisplayName));
        if (n.rfind(pat, 0) == 0 || d.find(pat) != std::string::npos) return true;
    }
    return false;
}

// ------------------------------------------------------------ power plans
const char* kBalanced        = "381b4222-f694-41f0-9685-ff5bb260df2e";
const char* kHighPerformance = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";
const char* kUltimate        = "e9a42b02-d5df-448d-aa00-03f14749eb61";

namespace {
std::string GuidText(const GUID& g) {
    char b[40];
    snprintf(b, sizeof(b), "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", (unsigned long)g.Data1, g.Data2, g.Data3,
             g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    return b;
}
bool ParseGuid(const std::string& s, GUID& g) {
    unsigned long d1; unsigned d2, d3, b[8];
    if (sscanf(s.c_str(), "%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x", &d1, &d2, &d3, &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &b[6], &b[7]) != 11) return false;
    g.Data1 = d1; g.Data2 = (unsigned short)d2; g.Data3 = (unsigned short)d3;
    for (int i = 0; i < 8; i++) g.Data4[i] = (unsigned char)b[i];
    return true;
}
}  // namespace

std::string ActivePlan() {
    GUID* g = nullptr;
    if (PowerGetActiveScheme(nullptr, &g) != ERROR_SUCCESS || !g) return {};
    std::string s = GuidText(*g);
    LocalFree(g);
    return s;
}

std::string PlanName(const std::string& guid) {
    GUID g;
    if (!ParseGuid(guid, g)) return "Unknown";
    DWORD size = 0;
    if (PowerReadFriendlyName(nullptr, &g, nullptr, nullptr, nullptr, &size) != ERROR_SUCCESS || !size) return "Unknown";
    std::vector<BYTE> buf(size);
    if (PowerReadFriendlyName(nullptr, &g, nullptr, nullptr, buf.data(), &size) != ERROR_SUCCESS) return "Unknown";
    return util::Narrow((const wchar_t*)buf.data());
}

bool PlanExists(const std::string& guid) {
    for (ULONG i = 0;; i++) {
        GUID g; DWORD size = sizeof(g);
        if (PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, i, (UCHAR*)&g, &size) != ERROR_SUCCESS) return false;
        if (GuidText(g) == util::Lower(guid)) return true;
    }
}

bool SetPlan(const std::string& guid) {
    GUID g;
    return ParseGuid(guid, g) && PowerSetActiveScheme(nullptr, &g) == ERROR_SUCCESS;
}

// ------------------------------------------------------------ launch priority
namespace {
const wchar_t* kIfeo = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
DWORD IfeoCode(const std::string& p) { return p == "High" ? 3 : p == "AboveNormal" ? 6 : 2; }

bool KeyEmpty(HKEY root, const std::wstring& path) {
    HKEY k;
    if (RegOpenKeyExW(root, path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS) return false;
    DWORD subkeys = 0, values = 0;
    RegQueryInfoKeyW(k, nullptr, nullptr, nullptr, &subkeys, nullptr, nullptr, &values, nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(k);
    return subkeys == 0 && values == 0;
}
}  // namespace

bool SetLaunchPriority(const std::string& exe, const std::string& priority) {
    std::wstring key = kIfeo + util::Widen(exe) + L"\\PerfOptions";
    HKEY k;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    DWORD v = IfeoCode(priority);
    bool ok = RegSetValueExW(k, L"CpuPriorityClass", 0, REG_DWORD, (const BYTE*)&v, sizeof(v)) == ERROR_SUCCESS;
    RegCloseKey(k);
    return ok;
}

bool HasLaunchPriority(const std::string& exe, const std::string& priority) {
    std::wstring key = kIfeo + util::Widen(exe) + L"\\PerfOptions";
    DWORD v = 0;
    return util::RegDword(HKEY_LOCAL_MACHINE, key.c_str(), L"CpuPriorityClass", v) && v == IfeoCode(priority);
}

void RemoveLaunchPriority(const std::string& exe) {
    std::wstring app = kIfeo + util::Widen(exe);
    std::wstring perf = app + L"\\PerfOptions";
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, perf.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &k) == ERROR_SUCCESS) {
        RegDeleteValueW(k, L"CpuPriorityClass");
        RegCloseKey(k);
    }
    // only remove keys we leave empty - other tools may keep settings there too
    if (KeyEmpty(HKEY_LOCAL_MACHINE, perf)) RegDeleteKeyExW(HKEY_LOCAL_MACHINE, perf.c_str(), KEY_WOW64_64KEY, 0);
    if (KeyEmpty(HKEY_LOCAL_MACHINE, app)) RegDeleteKeyExW(HKEY_LOCAL_MACHINE, app.c_str(), KEY_WOW64_64KEY, 0);
}

// ------------------------------------------------------------ GPU preference
bool PreferDedicatedGpu(const std::wstring& exePath) {
    const wchar_t* key = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";
    const wchar_t* want = L"GpuPreference=2;";
    wchar_t cur[128] = {};
    DWORD size = sizeof(cur);
    if (RegGetValueW(HKEY_CURRENT_USER, key, exePath.c_str(), RRF_RT_REG_SZ, nullptr, cur, &size) == ERROR_SUCCESS && wcscmp(cur, want) == 0)
        return false;
    return RegSetKeyValueW(HKEY_CURRENT_USER, key, exePath.c_str(), REG_SZ, want, (DWORD)((wcslen(want) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

std::wstring GpuPreference(const std::wstring& exePath) {
    wchar_t cur[256] = {};
    DWORD size = sizeof(cur);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\DirectX\\UserGpuPreferences", exePath.c_str(), RRF_RT_REG_SZ, nullptr, cur, &size) != ERROR_SUCCESS)
        return L"";
    return cur;
}

void SetGpuPreference(const std::wstring& exePath, const std::wstring& value) {
    const wchar_t* key = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";
    if (value.empty()) RegDeleteKeyValueW(HKEY_CURRENT_USER, key, exePath.c_str());
    else RegSetKeyValueW(HKEY_CURRENT_USER, key, exePath.c_str(), REG_SZ, value.c_str(), (DWORD)((value.size() + 1) * sizeof(wchar_t)));
}

// ------------------------------------------------------------ session tweaks
const GUID kSubProcessor   = { 0x54533251, 0x82be, 0x4824, { 0x96, 0xc1, 0x47, 0xb6, 0x0b, 0x74, 0x0d, 0x00 } };
const GUID kCoreParkingMin = { 0x0cc5b647, 0xc1df, 0x4637, { 0x89, 0x1a, 0xde, 0xc3, 0x5c, 0x31, 0x85, 0x83 } };
const GUID kIdleDisable    = { 0x5d76a2ca, 0xe8c0, 0x402f, { 0xa1, 0x33, 0x21, 0x58, 0x49, 0x2d, 0x58, 0xad } };
const GUID kBoostMode      = { 0xbe337238, 0x0d82, 0x4146, { 0xa9, 0x60, 0x4f, 0x37, 0x49, 0xd4, 0x70, 0xc7 } };
const GUID kEnergyPref     = { 0x36687f9e, 0xe3a5, 0x4dbf, { 0xb1, 0xdc, 0x15, 0xeb, 0x38, 0x1c, 0x68, 0x63 } };
const GUID kSubPciExpress  = { 0x501a4d13, 0x42af, 0x4429, { 0x9f, 0xd1, 0xa8, 0x21, 0x8c, 0x26, 0x8e, 0x20 } };
const GUID kLinkStatePower = { 0xee12f906, 0xd277, 0x404b, { 0xb6, 0xda, 0xe5, 0xfa, 0x1a, 0x57, 0x6d, 0xf5 } };   // ASPM: 0 off

namespace {
const char kSep = '|';

HKEY RootOf(const std::string& r) { return r == "HKLM" ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER; }
std::string RootName(HKEY r) { return r == HKEY_LOCAL_MACHINE ? "HKLM" : "HKCU"; }

std::vector<std::string> Fields(const std::string& s) {
    std::vector<std::string> f;
    size_t start = 0;
    for (;;) {
        size_t p = s.find(kSep, start);
        f.push_back(s.substr(start, p == std::string::npos ? std::string::npos : p - start));
        if (p == std::string::npos) break;
        start = p + 1;
    }
    return f;
}

// Runs a hidden PowerShell command; returns its exit code (-1 if it couldn't run)
int RunPowerShell(const std::wstring& command, DWORD timeoutMs = 20000) {
    std::wstring cmd = L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + command + L"\"";
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) return -1;
    DWORD code = (DWORD)-1;
    if (WaitForSingleObject(pi.hProcess, timeoutMs) == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &code);
    else TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (int)code;
}

std::wstring PsQuote(const std::wstring& s) {   // '...' with ' doubled
    std::wstring o = L"'";
    for (wchar_t c : s) { if (c == L'\'') o += L'\''; o += c; }
    return o + L"'";
}

GUID GuidOf(const std::string& s) { GUID g = {}; ParseGuid(s, g); return g; }
}  // namespace

std::string SetRegDword(HKEY root, const std::wstring& key, const std::wstring& value, DWORD data) {
    DWORD cur = 0;
    bool existed = util::RegDword(root, key.c_str(), value.c_str(), cur);
    if (existed && cur == data) return "";
    if (RegSetKeyValueW(root, key.c_str(), value.c_str(), REG_DWORD, &data, sizeof(data)) != ERROR_SUCCESS) return "";
    return "D|" + RootName(root) + "|" + util::Narrow(key) + "|" + util::Narrow(value) + "|" + (existed ? "1" : "0") + "|" + std::to_string(cur);
}

std::string SetRegString(HKEY root, const std::wstring& key, const std::wstring& value, const std::wstring& data) {
    wchar_t buf[512] = {};
    DWORD size = sizeof(buf);
    bool existed = RegGetValueW(root, key.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr, buf, &size) == ERROR_SUCCESS;
    if (existed && data == buf) return "";
    if (RegSetKeyValueW(root, key.c_str(), value.c_str(), REG_SZ, data.c_str(), (DWORD)((data.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS) return "";
    return "S|" + RootName(root) + "|" + util::Narrow(key) + "|" + util::Narrow(value) + "|" + (existed ? "1" : "0") + "|" + util::Narrow(buf);
}

std::string SetPowerValue(const GUID& sub, const GUID& setting, DWORD value) {
    GUID* scheme = nullptr;
    if (PowerGetActiveScheme(nullptr, &scheme) != ERROR_SUCCESS || !scheme) return "";
    GUID s = *scheme;
    LocalFree(scheme);
    DWORD cur = 0;
    if (PowerReadACValueIndex(nullptr, &s, &sub, &setting, &cur) != ERROR_SUCCESS) return "";
    if (cur == value) return "";
    if (PowerWriteACValueIndex(nullptr, &s, &sub, &setting, value) != ERROR_SUCCESS) return "";
    PowerSetActiveScheme(nullptr, &s);   // re-apply so the change takes effect now
    return "P|" + GuidText(s) + "|" + GuidText(sub) + "|" + GuidText(setting) + "|" + std::to_string(cur);
}

std::string SetVisualEffects(bool on) {
    BOOL client = TRUE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &client, 0);
    ANIMATIONINFO ai = { sizeof(ai) };
    SystemParametersInfoW(SPI_GETANIMATION, sizeof(ai), &ai, 0);
    if ((bool)client == on && (ai.iMinAnimate != 0) == on) return "";
    // no SPIF_UPDATEINIFILE: the change only lasts until sign-out, even if we crash
    SystemParametersInfoW(SPI_SETCLIENTAREAANIMATION, 0, (PVOID)(INT_PTR)(on ? TRUE : FALSE), SPIF_SENDCHANGE);
    ANIMATIONINFO set = { sizeof(set), on ? 1 : 0 };
    SystemParametersInfoW(SPI_SETANIMATION, sizeof(set), &set, SPIF_SENDCHANGE);
    return "V|" + std::to_string(client ? 1 : 0) + "|" + std::to_string(ai.iMinAnimate ? 1 : 0);
}

namespace {
const wchar_t* kPersonalize = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
void ThemeChanged() {   // Explorer and the taskbar re-read the setting
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ImmersiveColorSet", SMTO_ABORTIFHUNG, 1000, nullptr);
}
}  // namespace

std::string SetTransparency(bool on) {
    std::string b = SetRegDword(HKEY_CURRENT_USER, kPersonalize, L"EnableTransparency", on ? 1 : 0);
    if (!b.empty()) ThemeChanged();
    return b;
}

// Like the animations: no SPIF_UPDATEINIFILE, so it only lasts until sign-out even if we crash
std::string SetMouseAcceleration(bool on) {
    int m[3] = {};   // threshold 1, threshold 2, acceleration (0 = "Enhance pointer precision" off)
    if (!SystemParametersInfoW(SPI_GETMOUSE, 0, m, 0)) return "";
    if ((m[2] != 0) == on) return "";
    int set[3] = { on ? 6 : 0, on ? 10 : 0, on ? 1 : 0 };
    if (!SystemParametersInfoW(SPI_SETMOUSE, 0, set, SPIF_SENDCHANGE)) return "";
    return "M|" + std::to_string(m[0]) + "|" + std::to_string(m[1]) + "|" + std::to_string(m[2]);
}

std::string SetAccessibilityHotkeys(bool on) {
    STICKYKEYS sk = { sizeof(sk) };
    FILTERKEYS fk = { sizeof(fk) };
    TOGGLEKEYS tk = { sizeof(tk) };
    SystemParametersInfoW(SPI_GETSTICKYKEYS, sizeof(sk), &sk, 0);
    SystemParametersInfoW(SPI_GETFILTERKEYS, sizeof(fk), &fk, 0);
    SystemParametersInfoW(SPI_GETTOGGLEKEYS, sizeof(tk), &tk, 0);
    std::string b = "K|" + std::to_string(sk.dwFlags) + "|" + std::to_string(fk.dwFlags) + "|" + std::to_string(tk.dwFlags);
    bool changed = false;
    // only the shortcuts that pop up a dialog - a feature you've actually turned on is left alone
    auto hotkey = [&](DWORD& flags, DWORD featureOn, DWORD hotkeyBit) {
        if (flags & featureOn) return;
        DWORD want = on ? (flags | hotkeyBit) : (flags & ~hotkeyBit);
        if (want != flags) { flags = want; changed = true; }
    };
    hotkey(sk.dwFlags, SKF_STICKYKEYSON, SKF_HOTKEYACTIVE);
    hotkey(fk.dwFlags, FKF_FILTERKEYSON, FKF_HOTKEYACTIVE);
    hotkey(tk.dwFlags, TKF_TOGGLEKEYSON, TKF_HOTKEYACTIVE);
    if (!changed) return "";
    SystemParametersInfoW(SPI_SETSTICKYKEYS, sizeof(sk), &sk, SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETFILTERKEYS, sizeof(fk), &fk, SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETTOGGLEKEYS, sizeof(tk), &tk, SPIF_SENDCHANGE);
    return b;
}

std::string AddDefenderExclusion(const std::wstring& folder) {
    std::wstring q = PsQuote(folder);
    int rc = RunPowerShell(L"if ((Get-MpPreference).ExclusionPath -contains " + q + L") { exit 3 }; Add-MpPreference -ExclusionPath " + q + L"; exit 0");
    return rc == 0 ? "X|" + util::Narrow(folder) : "";   // 3 = you had already excluded it: leave it alone
}

namespace {
// Power mode overlays (Windows 10 1709+); these powrprof exports aren't in the SDK headers
typedef DWORD(WINAPI * GetOverlayFn)(GUID*);
typedef DWORD(WINAPI * SetOverlayFn)(GUID);
GetOverlayFn GetOverlay() { static auto f = (GetOverlayFn)GetProcAddress(LoadLibraryW(L"powrprof.dll"), "PowerGetEffectiveOverlayScheme"); return f; }
SetOverlayFn SetOverlay() { static auto f = (SetOverlayFn)GetProcAddress(LoadLibraryW(L"powrprof.dll"), "PowerSetActiveOverlayScheme"); return f; }
const char* kBestPerformanceMode = "ded574b5-45a0-4f42-8737-46345c09c238";
}  // namespace

std::string SetPowerMode(bool best) {
    if (!GetOverlay() || !SetOverlay()) return "";
    GUID cur = {};
    if (GetOverlay()(&cur) != ERROR_SUCCESS) cur = GUID{};   // no overlay = the default "Balanced" mode
    std::string curText = GuidText(cur);
    std::string want = best ? kBestPerformanceMode : "00000000-0000-0000-0000-000000000000";
    if (curText == want) return "";
    if (SetOverlay()(GuidOf(want)) != ERROR_SUCCESS) return "";
    return "O|" + curText;
}

void Restore(const std::string& b) {
    auto f = Fields(b);
    if (f.empty()) return;
    const std::string& t = f[0];
    if ((t == "D" || t == "S") && f.size() >= 6) {
        HKEY root = RootOf(f[1]);
        std::wstring key = util::Widen(f[2]), value = util::Widen(f[3]);
        if (f[4] != "1") {
            HKEY k;
            if (RegOpenKeyExW(root, key.c_str(), 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, value.c_str()); RegCloseKey(k); }
        } else if (t == "D") {
            DWORD d = (DWORD)strtoul(f[5].c_str(), nullptr, 10);
            RegSetKeyValueW(root, key.c_str(), value.c_str(), REG_DWORD, &d, sizeof(d));
        } else {
            std::wstring s = util::Widen(f[5]);
            RegSetKeyValueW(root, key.c_str(), value.c_str(), REG_SZ, s.c_str(), (DWORD)((s.size() + 1) * sizeof(wchar_t)));
        }
        if (key == kPersonalize) ThemeChanged();
    } else if (t == "P" && f.size() >= 5) {
        GUID scheme = GuidOf(f[1]), sub = GuidOf(f[2]), setting = GuidOf(f[3]);
        PowerWriteACValueIndex(nullptr, &scheme, &sub, &setting, (DWORD)strtoul(f[4].c_str(), nullptr, 10));
        if (ActivePlan() == f[1]) PowerSetActiveScheme(nullptr, &scheme);
    } else if (t == "V" && f.size() >= 3) {
        SystemParametersInfoW(SPI_SETCLIENTAREAANIMATION, 0, (PVOID)(INT_PTR)(f[1] == "1" ? TRUE : FALSE), SPIF_SENDCHANGE);
        ANIMATIONINFO ai = { sizeof(ai), f[2] == "1" ? 1 : 0 };
        SystemParametersInfoW(SPI_SETANIMATION, sizeof(ai), &ai, SPIF_SENDCHANGE);
    } else if (t == "M" && f.size() >= 4) {
        int m[3] = { atoi(f[1].c_str()), atoi(f[2].c_str()), atoi(f[3].c_str()) };
        SystemParametersInfoW(SPI_SETMOUSE, 0, m, SPIF_SENDCHANGE);
    } else if (t == "K" && f.size() >= 4) {
        STICKYKEYS sk = { sizeof(sk) }; FILTERKEYS fk = { sizeof(fk) }; TOGGLEKEYS tk = { sizeof(tk) };
        SystemParametersInfoW(SPI_GETSTICKYKEYS, sizeof(sk), &sk, 0);
        SystemParametersInfoW(SPI_GETFILTERKEYS, sizeof(fk), &fk, 0);
        SystemParametersInfoW(SPI_GETTOGGLEKEYS, sizeof(tk), &tk, 0);
        sk.dwFlags = strtoul(f[1].c_str(), nullptr, 10);
        fk.dwFlags = strtoul(f[2].c_str(), nullptr, 10);
        tk.dwFlags = strtoul(f[3].c_str(), nullptr, 10);
        SystemParametersInfoW(SPI_SETSTICKYKEYS, sizeof(sk), &sk, SPIF_SENDCHANGE);
        SystemParametersInfoW(SPI_SETFILTERKEYS, sizeof(fk), &fk, SPIF_SENDCHANGE);
        SystemParametersInfoW(SPI_SETTOGGLEKEYS, sizeof(tk), &tk, SPIF_SENDCHANGE);
    } else if (t == "O" && f.size() >= 2) {
        if (SetOverlay()) SetOverlay()(GuidOf(f[1]));
    } else if (t == "X" && f.size() >= 2) {
        RunPowerShell(L"Remove-MpPreference -ExclusionPath " + PsQuote(util::Widen(f[1])));
    }
}

bool SetTimerResolution(bool fine) {
    typedef LONG(NTAPI * Fn)(ULONG, BOOLEAN, PULONG);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetTimerResolution");
    ULONG cur = 0;
    return fn && fn(5000, fine ? TRUE : FALSE, &cur) == 0;   // 5000 x 100 ns = 0.5 ms
}

bool EnableGlobalTimerRequests() {
    // Windows 11 keeps timer requests per-process unless this is set (read at boot)
    return !SetRegDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel", L"GlobalTimerResolutionRequests", 1).empty();
}

bool SetFullscreenOptimizationsOff(const std::wstring& exePath, bool off) {
    const wchar_t* key = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    const std::wstring token = L"DISABLEDXMAXIMIZEDWINDOWEDMODE";
    wchar_t buf[512] = {};
    DWORD size = sizeof(buf);
    std::wstring cur = RegGetValueW(HKEY_CURRENT_USER, key, exePath.c_str(), RRF_RT_REG_SZ, nullptr, buf, &size) == ERROR_SUCCESS ? buf : L"";
    bool has = cur.find(token) != std::wstring::npos;
    if (has == off) return false;
    std::wstring next;
    if (off) next = cur.empty() ? L"~ " + token : cur + L" " + token;
    else {
        next = cur;
        size_t p = next.find(token);
        next.erase(p, token.size());
        while (next.find(L"  ") != std::wstring::npos) next.replace(next.find(L"  "), 2, L" ");
        while (!next.empty() && next.back() == L' ') next.pop_back();
    }
    if (next.empty() || next == L"~") {
        HKEY k;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS) return false;
        bool ok = RegDeleteValueW(k, exePath.c_str()) == ERROR_SUCCESS;
        RegCloseKey(k);
        return ok;
    }
    return RegSetKeyValueW(HKEY_CURRENT_USER, key, exePath.c_str(), REG_SZ, next.c_str(), (DWORD)((next.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

}  // namespace tweaks
