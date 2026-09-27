#include "driverinfo.h"
#include "util.h"
#include <filesystem>

namespace driverinfo {

namespace {
const wchar_t* kDisplayClass = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";

// the display class subkey ("0001") of the adapter with this name
std::wstring AdapterKey(const std::string& gpuName) {
    HKEY cls;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kDisplayClass, 0, KEY_READ, &cls) != ERROR_SUCCESS) return L"";
    std::wstring found;
    wchar_t sub[64];
    for (DWORD i = 0; found.empty(); i++) {
        DWORD n = 64;
        if (RegEnumKeyExW(cls, i, sub, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        if (util::RegString(cls, sub, L"DriverDesc") == gpuName) found = std::wstring(kDisplayClass) + L"\\" + sub;
    }
    RegCloseKey(cls);
    return found;
}

// AMD keeps many UMD settings as REG_BINARY holding a UTF-16 string ("1"); some are plain DWORDs
bool ReadNumber(const std::wstring& key, const wchar_t* value, long& out) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    BYTE buf[64] = {};
    DWORD type = 0, size = sizeof(buf) - 2;
    bool ok = RegQueryValueExW(k, value, nullptr, &type, buf, &size) == ERROR_SUCCESS;
    RegCloseKey(k);
    if (!ok) return false;
    if (type == REG_DWORD && size == 4) { out = (long)*(DWORD*)buf; return true; }
    if ((type == REG_BINARY || type == REG_SZ) && size >= 2) {
        std::wstring s((const wchar_t*)buf, size / 2);
        while (!s.empty() && s.back() == 0) s.pop_back();
        if (s.empty() || !iswdigit(s[0])) return false;
        out = wcstol(s.c_str(), nullptr, 10);
        return true;
    }
    return false;
}
}  // namespace

std::string Version(const std::string& gpuName) {
    std::wstring k = AdapterKey(gpuName);
    return k.empty() ? "" : util::RegString(HKEY_LOCAL_MACHINE, k.c_str(), L"DriverVersion");
}

std::vector<std::wstring> ShaderCacheDirs(const SystemInfo& sys) {
    std::wstring local = util::LocalAppDataRoot(), user = util::EnvVar(L"USERPROFILE"), pd = util::EnvVar(L"ProgramData");
    std::vector<std::wstring> dirs = { local + L"\\D3DSCache" };
    std::vector<std::string> makers;
    for (auto& g : sys.gpus) if (g.vendor != "Other" && !util::Contains(makers, g.vendor)) makers.push_back(g.vendor);
    if (util::Contains(makers, "AMD"))    for (auto d : { L"\\AMD\\DxCache", L"\\AMD\\DxcCache", L"\\AMD\\VkCache", L"\\AMD\\GLCache" }) dirs.push_back(local + d);
    if (util::Contains(makers, "NVIDIA")) { dirs.push_back(local + L"\\NVIDIA\\DXCache"); dirs.push_back(local + L"\\NVIDIA\\GLCache"); dirs.push_back(pd + L"\\NVIDIA Corporation\\NV_Cache"); }
    if (util::Contains(makers, "Intel"))  { dirs.push_back(local + L"\\Intel\\ShaderCache"); dirs.push_back(user + L"\\AppData\\LocalLow\\Intel\\ShaderCache"); }
    return dirs;
}

CacheSize ShaderCacheSize(const std::vector<std::wstring>& dirs) {
    namespace fs = std::filesystem;
    CacheSize c;
    for (auto& d : dirs) {
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        for (auto it = fs::recursive_directory_iterator(d, fs::directory_options::skip_permission_denied, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            uint64_t n = it->file_size(ec);
            if (!ec) { c.bytes += n; c.files++; }
        }
    }
    return c;
}

std::vector<Setting> AmdSettings(const SystemInfo& sys) {
    std::vector<Setting> out;
    const GpuInfo* g = nullptr;
    for (auto& x : sys.gpus) if (x.vendor == "AMD" && !g) g = &x;
    if (!g) return out;
    std::wstring key = AdapterKey(g->name);
    if (key.empty()) return out;
    long v = 0;
    if (ReadNumber(key, L"KMD_ChillEnabled", v))
        out.push_back({ "Radeon Chill", v ? "On" : "Off",
                        v ? "Lowers your FPS when you're not moving to save power - it can feel sluggish in fast games. Turn it off in Adrenalin for competitive games." : "",
                        v != 0 });
    if (ReadNumber(key, L"KMD_DeLagEnabled", v))
        out.push_back({ "Radeon Anti-Lag", v ? "On" : "Off",
                        v ? "" : "Anti-Lag cuts input lag when the graphics card is the limit (GPU at 95-100%). Worth trying in Adrenalin > Gaming > Graphics.",
                        false });
    if (ReadNumber(key, L"KMD_RadeonBoostEnabled", v))
        out.push_back({ "Radeon Boost", v ? "On" : "Off",
                        v ? "Lowers the resolution during fast motion for more FPS - the picture gets softer while you move." : "", v != 0 });
    if (ReadNumber(key + L"\\UMD", L"VSyncControl", v)) {
        const char* names[] = { "Always off", "Off, unless the game asks", "On, unless the game asks", "Always on" };
        out.push_back({ "Wait for vertical refresh", v >= 0 && v <= 3 ? names[v] : std::to_string(v),
                        v == 3 ? "Forced VSync adds input lag. With a FreeSync screen, an FPS cap a little under your refresh rate is smoother and more responsive." : "",
                        v == 3 });
    }
    return out;
}

}  // namespace driverinfo
