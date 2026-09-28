#include "processes.h"
#include "util.h"
#include <tlhelp32.h>

// ------------------------------------------------------------ ProcessList
void ProcessList::Refresh() {
    byName_.clear();
    names_.clear();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe = { sizeof(pe) };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        std::string n = util::StripExe(util::Narrow(pe.szExeFile));
        names_[pe.th32ProcessID] = n;
        byName_[util::Lower(n)].push_back(pe.th32ProcessID);
    }
    CloseHandle(snap);
}

std::vector<DWORD> ProcessList::Find(const std::string& name) const {
    auto it = byName_.find(util::Lower(util::StripExe(name)));
    return it == byName_.end() ? std::vector<DWORD>() : it->second;
}

std::string ProcessList::NameOf(DWORD pid) const {
    auto it = names_.find(pid);
    return it == names_.end() ? std::string() : it->second;
}

std::vector<DWORD> ProcessList::All() const {
    std::vector<DWORD> v;
    for (auto& [pid, n] : names_) v.push_back(pid);
    return v;
}

// ------------------------------------------------------------ per-process
namespace proc {

namespace {
struct Handle {
    HANDLE h;
    Handle(DWORD access, DWORD pid) : h(OpenProcess(access, FALSE, pid)) {}
    ~Handle() { if (h) CloseHandle(h); }
    explicit operator bool() const { return h != nullptr; }
};
}  // namespace

uint64_t CreationTime(DWORD pid) {
    Handle h(PROCESS_QUERY_LIMITED_INFORMATION, pid);
    if (!h) return 0;
    FILETIME c, e, k, u;
    if (!GetProcessTimes(h.h, &c, &e, &k, &u)) return 0;
    DWORD code = 0;
    if (GetExitCodeProcess(h.h, &code) && code != STILL_ACTIVE) return 0;
    return ((uint64_t)c.dwHighDateTime << 32) | c.dwLowDateTime;
}

bool Alive(const ProcKey& k) { return k.created && CreationTime(k.pid) == k.created; }

std::wstring ImagePath(DWORD pid) {
    Handle h(PROCESS_QUERY_LIMITED_INFORMATION, pid);
    if (!h) return {};
    wchar_t buf[MAX_PATH * 2];
    DWORD n = MAX_PATH * 2;
    return QueryFullProcessImageNameW(h.h, 0, buf, &n) ? std::wstring(buf, n) : std::wstring();
}

DWORD PriorityClassOf(const std::string& name) {
    if (name == "High") return HIGH_PRIORITY_CLASS;
    if (name == "AboveNormal") return ABOVE_NORMAL_PRIORITY_CLASS;
    if (name == "BelowNormal") return BELOW_NORMAL_PRIORITY_CLASS;
    if (name == "Idle") return IDLE_PRIORITY_CLASS;
    return NORMAL_PRIORITY_CLASS;
}

bool GetPriority(DWORD pid, DWORD& cls) {
    Handle h(PROCESS_QUERY_LIMITED_INFORMATION, pid);
    if (!h) return false;
    cls = GetPriorityClass(h.h);
    return cls != 0;
}

bool SetPriority(DWORD pid, DWORD cls) {
    Handle h(PROCESS_SET_INFORMATION, pid);
    return h && SetPriorityClass(h.h, cls);
}

bool GetAffinity(DWORD pid, uint64_t& mask) {
    Handle h(PROCESS_QUERY_LIMITED_INFORMATION, pid);
    DWORD_PTR pm = 0, sm = 0;
    if (!h || !GetProcessAffinityMask(h.h, &pm, &sm)) return false;
    mask = pm;
    return true;
}

bool SetAffinity(DWORD pid, uint64_t mask) {
    Handle h(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, pid);
    return h && mask && SetProcessAffinityMask(h.h, (DWORD_PTR)mask);
}

bool SetEcoQosOff(DWORD pid, bool off) {
    // PROCESS_POWER_THROTTLING_STATE through SetProcessInformation(ProcessPowerThrottling = 4)
    struct { ULONG Version, ControlMask, StateMask; } s = { 1, off ? 1u : 0u, 0 };
    typedef BOOL(WINAPI * Fn)(HANDLE, int, LPVOID, DWORD);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetProcessInformation");
    if (!fn) return false;
    Handle h(PROCESS_SET_INFORMATION, pid);
    return h && fn(h.h, 4, &s, sizeof(s));
}

namespace {
struct CloseCtx { DWORD pid; int sent; };
BOOL CALLBACK CloseWindows(HWND w, LPARAM lp) {
    auto* c = (CloseCtx*)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(w, &pid);
    if (pid == c->pid && IsWindowVisible(w) && !GetWindow(w, GW_OWNER)) {
        PostMessageW(w, WM_CLOSE, 0, 0);
        c->sent++;
    }
    return TRUE;
}
}  // namespace

bool Close(DWORD pid) {
    CloseCtx c = { pid, 0 };
    EnumWindows(CloseWindows, (LPARAM)&c);
    if (c.sent) return true;                    // asked nicely, like closing its window
    Handle h(PROCESS_TERMINATE, pid);           // no window (tray apps, helpers): end it
    return h && TerminateProcess(h.h, 0);
}

bool SetIoPriorityHigh(DWORD pid) {
    typedef LONG(NTAPI * Fn)(HANDLE, int, PVOID, ULONG);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetInformationProcess");
    if (!fn) return false;
    EnablePrivilege(L"SeIncreaseBasePriorityPrivilege");
    Handle h(PROCESS_SET_INFORMATION, pid);
    ULONG prio = 3;                               // IoPriorityHigh
    return h && fn(h.h, 33, &prio, sizeof(prio)) == 0;   // ProcessIoPriority
}

bool SetEcoQosOn(DWORD pid, bool on) {
    // PROCESS_POWER_THROTTLING_EXECUTION_SPEED in both masks = "throttle me" (efficiency mode)
    struct { ULONG Version, ControlMask, StateMask; } s = { 1, on ? 1u : 0u, on ? 1u : 0u };
    typedef BOOL(WINAPI * Fn)(HANDLE, int, LPVOID, DWORD);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetProcessInformation");
    if (!fn) return false;
    Handle h(PROCESS_SET_INFORMATION, pid);
    return h && fn(h.h, 4, &s, sizeof(s));
}

bool ClearCpuSets(DWORD pid) {
    Handle h(PROCESS_SET_LIMITED_INFORMATION, pid);
    return h && SetProcessDefaultCpuSets(h.h, nullptr, 0);
}

bool SetCpuSets(DWORD pid, uint64_t mask, bool onePerCore) {
    ULONG len = 0;
    GetSystemCpuSetInformation(nullptr, 0, &len, nullptr, 0);
    if (!len) return false;
    std::vector<BYTE> buf(len);
    if (!GetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)buf.data(), len, &len, nullptr, 0)) return false;
    std::vector<ULONG> ids;
    std::vector<std::pair<BYTE, BYTE>> seenCores;          // (group, core index)
    for (ULONG off = 0; off < len;) {
        auto* e = (PSYSTEM_CPU_SET_INFORMATION)(buf.data() + off);
        if (!e->Size) break;
        if (e->Type == CpuSetInformation && e->CpuSet.Group == 0 && e->CpuSet.LogicalProcessorIndex < 64 &&
            ((mask >> e->CpuSet.LogicalProcessorIndex) & 1)) {
            auto core = std::make_pair((BYTE)e->CpuSet.Group, e->CpuSet.CoreIndex);
            bool seen = false;
            for (auto& c : seenCores) if (c == core) seen = true;
            if (!onePerCore || !seen) { seenCores.push_back(core); ids.push_back(e->CpuSet.Id); }
        }
        off += e->Size;
    }
    if (ids.empty()) return false;
    Handle h(PROCESS_SET_LIMITED_INFORMATION, pid);
    return h && SetProcessDefaultCpuSets(h.h, ids.data(), (ULONG)ids.size());
}

bool TrimWorkingSet(DWORD pid) {
    Handle h(PROCESS_SET_QUOTA | PROCESS_QUERY_LIMITED_INFORMATION, pid);
    return h && SetProcessWorkingSetSize(h.h, (SIZE_T)-1, (SIZE_T)-1);
}

bool EnablePrivilege(const wchar_t* name) {
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool ok = LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid) &&
              AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr) && GetLastError() == ERROR_SUCCESS;
    CloseHandle(token);
    return ok;
}

uint32_t PurgeStandbyList() {
    typedef LONG(NTAPI * Fn)(int, PVOID, ULONG);
    static Fn fn = (Fn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtSetSystemInformation");
    if (!fn) return 0xFFFFFFFF;
    EnablePrivilege(L"SeProfileSingleProcessPrivilege");
    int cmd = 4;                                 // MemoryPurgeStandbyList
    return (uint32_t)fn(80, &cmd, sizeof(cmd));  // SystemMemoryListInformation
}

}  // namespace proc

namespace proc {
uint64_t ReadBytes(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return 0;
    IO_COUNTERS io = {};
    bool ok = GetProcessIoCounters(h, &io) != 0;
    CloseHandle(h);
    return ok ? io.ReadTransferCount : 0;
}
}  // namespace proc
