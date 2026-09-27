#include "updater.h"
#include "json.h"
#include "util.h"
#include "version.h"
#include <cstdio>
#include <windows.h>
#include <bcrypt.h>
#include <winhttp.h>

// ------------------------------------------------------------ HTTP
namespace net {

bool Get(const std::string& url, std::string& body, std::string& error, int timeoutSec) {
    body.clear();
    std::wstring wurl = util::Widen(url);
    URL_COMPONENTS uc = { sizeof(uc) };
    wchar_t host[256], path[2048];
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 2048;
    wchar_t extra[2048]; uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) { error = "bad URL"; return false; }
    std::wstring target = std::wstring(path, uc.dwUrlPathLength) + std::wstring(extra, uc.dwExtraInfoLength);

    std::wstring agent = L"ProjectOptM/" + util::Widen(OPTM_VERSION);
    HINTERNET s = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) s = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) { error = "no network"; return false; }
    int ms = timeoutSec * 1000;
    WinHttpSetTimeouts(s, ms, ms, ms, ms);
    HINTERNET c = WinHttpConnect(s, std::wstring(host, uc.dwHostNameLength).c_str(), uc.nPort, 0);
    HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", target.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                         uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) : nullptr;
    bool ok = false;
    if (r) {
        const wchar_t* hdr = L"Accept: application/vnd.github+json, application/octet-stream, */*\r\n";
        if (WinHttpSendRequest(r, hdr, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(r, nullptr)) {
            DWORD code = 0, size = sizeof(code);
            WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &size, WINHTTP_NO_HEADER_INDEX);
            for (;;) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(r, &avail) || !avail) break;
                size_t at = body.size();
                body.resize(at + avail);
                DWORD got = 0;
                if (!WinHttpReadData(r, &body[at], avail, &got)) { body.resize(at); break; }
                body.resize(at + got);
            }
            ok = code == 200;
            if (!ok) error = "server answered " + std::to_string(code);
        } else {
            error = "couldn't connect (error " + std::to_string(GetLastError()) + ")";
        }
        WinHttpCloseHandle(r);
    } else if (error.empty()) {
        error = "couldn't connect";
    }
    if (c) WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);
    return ok;
}

}  // namespace net

namespace {

std::string Sha256Hex(const std::string& data) {
    BYTE hash[32];
    if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, (PUCHAR)data.data(), (ULONG)data.size(), hash, 32) != 0) return {};
    std::string hex;
    char b[3];
    for (BYTE x : hash) { snprintf(b, sizeof(b), "%02x", x); hex += b; }
    return hex;
}

// The running version. OPTM_UPDATE_AS pretends to be an older one (testing the updater).
std::string Current() {
    static std::string v = [] { std::string e = util::Narrow(util::EnvVar(L"OPTM_UPDATE_AS")); return e.empty() ? std::string(OPTM_VERSION) : e; }();
    return v;
}

std::vector<int> Parts(std::string v) {
    while (!v.empty() && (v[0] == 'v' || v[0] == 'V' || v[0] == ' ')) v.erase(0, 1);
    std::vector<int> p;
    for (auto& x : util::Split(v, '.')) p.push_back(atoi(x.c_str()));
    while (p.size() < 4) p.push_back(0);
    return p;
}

}  // namespace

// ------------------------------------------------------------ Updater
Updater::~Updater() { Join(); }

void Updater::Join() { if (worker_.joinable()) worker_.join(); }

bool Updater::Enabled() const { return OPTM_UPDATE_REPO[0] != 0; }

bool Updater::Newer(const std::string& tag, const std::string& current) {
    return Parts(tag) > Parts(current);
}

void Updater::Set(State s, const std::string& status) { std::lock_guard<std::mutex> l(mu_); state_ = s; status_ = status; }
void Updater::Say(const std::string& line) { std::lock_guard<std::mutex> l(mu_); log_.push_back(line); }

Updater::State Updater::GetState() const { std::lock_guard<std::mutex> l(mu_); return state_; }
std::string Updater::Status() const { std::lock_guard<std::mutex> l(mu_); return status_; }
std::string Updater::Version() const { std::lock_guard<std::mutex> l(mu_); return version_; }
std::string Updater::Notes() const { std::lock_guard<std::mutex> l(mu_); return notes_; }
std::vector<std::string> Updater::TakeLog() { std::lock_guard<std::mutex> l(mu_); auto v = std::move(log_); log_.clear(); return v; }

void Updater::Check(bool manual) {
    State s = GetState();
    if (!Enabled() || s == Checking || s == Downloading || s == Ready) return;
    Join();
    Set(Checking, "Checking for updates...");
    worker_ = std::thread([this, manual] { DoCheck(manual); });
}

void Updater::DoCheck(bool manual) {
    std::string body, err;
    if (!net::Get(OPTM_UPDATE_API, body, err, 10)) {
        std::lock_guard<std::mutex> l(mu_);
        state_ = version_.empty() ? Failed : Available;   // keep offering an update we already found
        if (version_.empty()) status_ = "Couldn't check for updates";
        if (manual) log_.push_back("Update check failed: " + err);
        return;
    }
    Json rel = Json::Parse(body);
    std::string tag = rel["tag_name"].AsString();
    const Json* asset = nullptr;
    for (auto& a : rel["assets"].arr)
        if (a["name"].AsString() == "ProjectOptM.exe") asset = &a;
    std::lock_guard<std::mutex> l(mu_);
    if (asset && Newer(tag, Current())) {
        std::string ver = tag;
        while (!ver.empty() && (ver[0] == 'v' || ver[0] == 'V')) ver.erase(0, 1);
        bool isNew = version_ != ver;
        version_ = ver;
        url_ = (*asset)["browser_download_url"].AsString();
        digest_ = (*asset)["digest"].AsString();
        notes_ = rel["body"].AsString();
        state_ = Available;
        status_ = "Version " + ver + " is available.";
        if (isNew) log_.push_back("Update available: v" + ver + " - click 'Update to v" + ver + "' at the top");
    } else {
        state_ = UpToDate;
        status_ = "Up to date (checked " + util::NowStamp("%H:%M") + ")";
        if (manual) log_.push_back("You're on the latest version (v" + Current() + ")");
    }
}

void Updater::Install() {
    if (GetState() != Available) return;
    Join();
    Set(Downloading, "Downloading...");
    worker_ = std::thread([this] { DoInstall(); });
}

void Updater::DoInstall() {
    std::string ver, url, digest;
    { std::lock_guard<std::mutex> l(mu_); ver = version_; url = url_; digest = digest_; }
    auto fail = [&](const std::string& why) {
        Say("Update failed: " + why + ". Nothing was changed.");
        Set(Available, "Version " + ver + " is available.");
    };
    Say("Downloading v" + ver + "...");
    std::string data, err;
    if (!net::Get(url, data, err, 120)) return fail(err);
    if (digest.rfind("sha256:", 0) == 0 && util::Lower(digest.substr(7)) != Sha256Hex(data))
        return fail("the download was corrupted (checksum mismatch)");
    if (data.size() < 20000 || data[0] != 'M' || data[1] != 'Z') return fail("the downloaded file isn't a valid app");

    std::wstring self = util::SelfPath();
    std::wstring tmp = util::EnvVar(L"TEMP") + L"\\ProjectOptM-" + util::Widen(ver) + L".exe";
    if (!util::WriteFile(tmp, data)) return fail("couldn't save the download");
    // keep the current version, then swap: a running exe can't be overwritten, but it can be renamed
    std::wstring bak = util::AppDataDir() + L"\\backup";
    CreateDirectoryW(util::AppDataDir().c_str(), nullptr);
    CreateDirectoryW(bak.c_str(), nullptr);
    CopyFileW(self.c_str(), (bak + L"\\ProjectOptM-v" OPTM_VERSION L".exe").c_str(), FALSE);
    std::wstring old = self + L".old";
    DeleteFileW(old.c_str());
    if (!MoveFileExW(self.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING)) { DeleteFileW(tmp.c_str()); return fail("couldn't replace the app file"); }
    if (!CopyFileW(tmp.c_str(), self.c_str(), FALSE)) {
        MoveFileExW(old.c_str(), self.c_str(), MOVEFILE_REPLACE_EXISTING);
        DeleteFileW(tmp.c_str());
        return fail("couldn't write the new version");
    }
    DeleteFileW(tmp.c_str());
    Say("Updated to v" + ver + " - restarting...");
    Set(Ready, "Restarting...");
}
