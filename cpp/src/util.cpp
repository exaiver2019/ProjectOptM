#include "util.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <shellapi.h>
#include <shlobj.h>

namespace util {

std::string Narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring Widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
    return s;
}

std::vector<std::string> Split(const std::string& s, char sep) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= s.size()) {
        size_t end = s.find(sep, start);
        if (end == std::string::npos) end = s.size();
        std::string part = Trim(s.substr(start, end - start));
        if (!part.empty()) out.push_back(part);
        start = end + 1;
    }
    return out;
}

std::string Join(const std::vector<std::string>& v, const char* sep) {
    std::string o;
    for (size_t i = 0; i < v.size(); i++) { if (i) o += sep; o += v[i]; }
    return o;
}

bool Contains(const std::vector<std::string>& v, const std::string& s) {
    std::string l = Lower(s);
    for (auto& x : v) if (Lower(x) == l) return true;
    return false;
}

std::string StripExe(std::string name) {
    if (name.size() > 4 && Lower(name.substr(name.size() - 4)) == ".exe") name.resize(name.size() - 4);
    return name;
}

std::vector<std::string> NameList(const std::string& s) {
    std::vector<std::string> out;
    for (auto& e : Split(s, ',')) {
        std::string n = StripExe(e);
        if (!Contains(out, n)) out.push_back(n);
    }
    return out;
}

bool ReadFile(const std::wstring& path, std::string& out) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? n : 0);
    if (n > 0) out.resize(fread(out.data(), 1, n, f));
    fclose(f);
    if (out.size() >= 3 && (unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF)
        out.erase(0, 3);   // UTF-8 BOM
    return true;
}

bool WriteFile(const std::wstring& path, const std::string& data) {
    // write next to it and swap in, so a crash never leaves a half-written file
    std::wstring tmp = path + L".tmp";
    FILE* f = _wfopen(tmp.c_str(), L"wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    ok = fclose(f) == 0 && ok;
    if (ok) ok = MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
    if (!ok) DeleteFileW(tmp.c_str());
    return ok;
}

bool AppendFile(const std::wstring& path, const std::string& data) {
    FILE* f = _wfopen(path.c_str(), L"ab");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    return fclose(f) == 0 && ok;
}

int64_t FileTime(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &a)) return 0;
    return ((int64_t)a.ftLastWriteTime.dwHighDateTime << 32) | a.ftLastWriteTime.dwLowDateTime;
}

static std::wstring Folder(int csidl) {
    wchar_t buf[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, csidl, nullptr, 0, buf);
    return buf;
}
std::wstring AppDataRoot() { return Folder(CSIDL_APPDATA); }
std::wstring LocalAppDataRoot() { return Folder(CSIDL_LOCAL_APPDATA); }
std::wstring AppDataDir() {
    static std::wstring over = EnvVar(L"OPTM_DATA_DIR");   // testing: use a copy of the data folder
    return over.empty() ? AppDataRoot() + L"\\ProjectOptM" : over;
}

std::wstring SelfPath() {
    std::wstring buf(MAX_PATH, L'\0');
    for (;;) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), (DWORD)buf.size());
        if (n < buf.size()) { buf.resize(n); return buf; }
        buf.resize(buf.size() * 2);
    }
}

std::wstring EnvVar(const wchar_t* name) {
    wchar_t buf[1024];
    DWORD n = GetEnvironmentVariableW(name, buf, 1024);
    return (n && n < 1024) ? std::wstring(buf, n) : std::wstring();
}

std::string RegString(HKEY root, const wchar_t* key, const wchar_t* value) {
    wchar_t buf[512] = {};
    DWORD size = sizeof(buf);
    if (RegGetValueW(root, key, value, RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS) return {};
    return Trim(Narrow(buf));
}

bool RegDword(HKEY root, const wchar_t* key, const wchar_t* value, DWORD& out) {
    DWORD size = sizeof(out);
    return RegGetValueW(root, key, value, RRF_RT_REG_DWORD, nullptr, &out, &size) == ERROR_SUCCESS;
}

bool SetRegDword(HKEY root, const wchar_t* key, const wchar_t* value, DWORD data) {
    return RegSetKeyValueW(root, key, value, REG_DWORD, &data, sizeof(data)) == ERROR_SUCCESS;
}

int AppxInstalled(const wchar_t* prefix) {
    HKEY k;
    const wchar_t* path = L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &k) != ERROR_SUCCESS) return -1;
    int found = 0;
    size_t n = wcslen(prefix);
    wchar_t name[512];
    for (DWORD i = 0;; i++) {
        DWORD len = 512;
        if (RegEnumKeyExW(k, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        if (_wcsnicmp(name, prefix, n) == 0) { found = 1; break; }
    }
    RegCloseKey(k);
    return found;
}

std::string FormatHours(double minutes) {
    char b[32];
    if (minutes >= 60) snprintf(b, sizeof(b), "%.1fh", minutes / 60.0);
    else snprintf(b, sizeof(b), "%.0fm", minutes);
    return b;
}

std::string FormatDuration(double minutes) {
    char b[32];
    if (minutes >= 60) snprintf(b, sizeof(b), "%dh %dm", (int)(minutes / 60), (int)fmod(minutes, 60));
    else snprintf(b, sizeof(b), "%dm", std::max(1, (int)(minutes + 0.5)));
    return b;
}

std::string NowStamp(const char* fmt) {
    time_t t = time(nullptr); tm lt; localtime_s(&lt, &t);
    char b[64]; strftime(b, sizeof(b), fmt, &lt);
    return b;
}

void OpenAsUser(const std::wstring& target) {
    // Explorer runs as the signed-in user, so games and links don't inherit our admin rights
    std::wstring args = L"\"" + target + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

}  // namespace util
