// Running processes and the per-process tweaks (priority, affinity, efficiency mode).
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

// A process identity that survives PID reuse: PID + creation time.
struct ProcKey {
    DWORD pid = 0;
    uint64_t created = 0;
    bool operator<(const ProcKey& o) const { return pid != o.pid ? pid < o.pid : created < o.created; }
};

class ProcessList {
public:
    void Refresh();
    std::vector<DWORD> Find(const std::string& name) const;   // exe name without ".exe", any case
    bool Has(const std::string& name) const { return !Find(name).empty(); }
    std::string NameOf(DWORD pid) const;                      // as Windows reports it, without ".exe"
    std::vector<DWORD> All() const;

private:
    std::map<std::string, std::vector<DWORD>> byName_;        // lower-case name -> PIDs
    std::map<DWORD, std::string> names_;
};

namespace proc {

uint64_t     CreationTime(DWORD pid);                // 0 if the process is gone / inaccessible
bool         Alive(const ProcKey& k);                // still the same process
std::wstring ImagePath(DWORD pid);                   // full exe path, empty if unavailable
DWORD        PriorityClassOf(const std::string& name);   // "Normal"/"AboveNormal"/"High"/"BelowNormal"/"Idle"
bool         GetPriority(DWORD pid, DWORD& cls);
bool         SetPriority(DWORD pid, DWORD cls);
bool         GetAffinity(DWORD pid, uint64_t& mask);
bool         SetAffinity(DWORD pid, uint64_t mask);
bool         SetEcoQosOff(DWORD pid, bool off);      // off = never throttle; false hands control back to Windows
bool         Close(DWORD pid);                       // WM_CLOSE to its windows, or terminate if it has none
bool         SetIoPriorityHigh(DWORD pid);
// CPU sets: the game prefers the CPUs in mask (soft pinning). onePerCore keeps only the
// first logical CPU of each physical core (SMT scheduling).
bool         SetCpuSets(DWORD pid, uint64_t mask, bool onePerCore);
bool         ClearCpuSets(DWORD pid);                // back to all CPUs
bool         SetEcoQosOn(DWORD pid, bool on);        // efficiency mode (like Task Manager's); false hands control back
bool         TrimWorkingSet(DWORD pid);
bool         EnablePrivilege(const wchar_t* name);
uint32_t     PurgeStandbyList();                     // NTSTATUS, 0 = success
uint64_t     ReadBytes(DWORD pid);                   // bytes the process has read so far (query rights only; 0 if unavailable)

}  // namespace proc
