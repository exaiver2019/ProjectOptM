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

// The running version. OPTM_UPDATE_AS pretends to be another one (testing the updater).
std::string Current() {
    static std::string v = [] { std::string e = util::Narrow(util::EnvVar(L"OPTM_UPDATE_AS")); return e.empty() ? Updater::CurrentTag() : e; }();
    return v;
}

// "v2.1.1-experimental.3" -> numbers {2,1,1,0}, channel rank (stable 2, experimental 1, unstable 0), pre-release number
struct Key {
    std::vector<int> nums;
    int rank = 2, pre = 0;
    bool operator<(const Key& o) const {
        if (nums != o.nums) return nums < o.nums;
        if (rank != o.rank) return rank < o.rank;
        return pre < o.pre;
    }
};

Key Parse(std::string v) {
    while (!v.empty() && (v[0] == 'v' || v[0] == 'V' || v[0] == ' ')) v.erase(0, 1);
    Key k;
    size_t dash = v.find('-');
    std::string num = v.substr(0, dash), suffix = dash == std::string::npos ? "" : util::Lower(v.substr(dash + 1));
    for (auto& x : util::Split(num, '.')) k.nums.push_back(atoi(x.c_str()));
    while (k.nums.size() < 4) k.nums.push_back(0);
    if (!suffix.empty()) {
        k.rank = suffix.rfind("unstable", 0) == 0 ? 0 : 1;   // any other suffix (beta, rc, experimental) counts as a pre-release
        size_t dot = suffix.find_last_of('.');
        if (dot != std::string::npos) k.pre = atoi(suffix.c_str() + dot + 1);
    }
    return k;
}

// Where releases come from. Test copies can point it at a local server (OPTM_TEST_UPDATE_BASE).
std::string ApiBase() {
    std::wstring t = util::EnvVar(L"OPTM_TEST_UPDATE_BASE");
    if (!t.empty() && !util::EnvVar(L"OPTM_DATA_DIR").empty()) return util::Narrow(t);
    return "https://api.github.com/repos/" OPTM_UPDATE_REPO;
}

}  // namespace

std::string Updater::CurrentTag() {
    if (!OPTM_CHANNEL[0]) return OPTM_VERSION;
    return std::string(OPTM_VERSION) + "-" + OPTM_CHANNEL + "." + std::to_string(OPTM_PRERELEASE);
}

void Updater::SetChannel(const std::string& channel) {
    std::lock_guard<std::mutex> l(mu_);
    channel_ = channel == "experimental" ? "experimental" : "stable";
}

// ------------------------------------------------------------ Updater
Updater::~Updater() { Join(); }

void Updater::Join() { if (worker_.joinable()) worker_.join(); }

bool Updater::Enabled() const { return OPTM_UPDATE_REPO[0] != 0; }

bool Updater::Newer(const std::string& tag, const std::string& current) {
    return Parse(current) < Parse(tag);
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
    std::string channel;
    { std::lock_guard<std::mutex> l(mu_); channel = channel_; }
    bool pre = channel == "experimental";
    // stable: the latest full release (GitHub never marks a pre-release "latest");
    // experimental: the newest of the recent releases, pre-releases included
    bool testBase = !util::EnvVar(L"OPTM_TEST_UPDATE_BASE").empty() && !util::EnvVar(L"OPTM_DATA_DIR").empty();
    std::string url = pre ? ApiBase() + "/releases?per_page=20" : testBase ? ApiBase() + "/releases/latest" : std::string(OPTM_UPDATE_API);
    std::string body, err;
    if (!net::Get(url, body, err, 10)) {
        std::lock_guard<std::mutex> l(mu_);
        state_ = version_.empty() ? Failed : Available;   // keep offering an update we already found
        if (version_.empty()) status_ = "Couldn't check for updates";
        if (manual) log_.push_back("Update check failed: " + err);
        return;
    }
    Json list = Json::Parse(body);
    std::vector<const Json*> candidates;
    if (pre) { for (auto& r : list.arr) candidates.push_back(&r); }
    else candidates.push_back(&list);
    const Json* best = nullptr;
    const Json* asset = nullptr;
    std::string tag;
    for (auto* r : candidates) {
        if ((*r)["draft"].AsBool(false)) continue;
        std::string t = (*r)["tag_name"].AsString();
        if (t.empty() || Parse(t).rank == 0) continue;   // unstable builds are never offered
        const Json* a = nullptr;
        for (auto& x : (*r)["assets"].arr)
            if (x["name"].AsString() == "ProjectOptM.exe") a = &x;
        if (!a) continue;
        if (!best || Parse(tag) < Parse(t)) { best = r; asset = a; tag = t; }
    }
    const Json& rel = best ? *best : list;
    std::lock_guard<std::mutex> l(mu_);
    if (asset && Newer(tag, Current())) {
        prerelease_ = rel["prerelease"].AsBool(false);
        std::string ver = tag;
        while (!ver.empty() && (ver[0] == 'v' || ver[0] == 'V')) ver.erase(0, 1);
        bool isNew = version_ != ver;
        version_ = ver;
        url_ = (*asset)["browser_download_url"].AsString();
        digest_ = (*asset)["digest"].AsString();
        notes_ = rel["body"].AsString();
        state_ = Available;
        status_ = std::string(prerelease_ ? "Experimental build " : "Version ") + ver + " is available.";
        if (isNew) log_.push_back(std::string(prerelease_ ? "Experimental update" : "Update") + " available: v" + ver + " - click 'Update to v" + ver + "' at the top");
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
