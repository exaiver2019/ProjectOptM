// Did the game crash? Two sources, neither touches the game:
//  - its exit code (a handle opened with query rights only, like Task Manager's) - not for anti-cheat games
//  - Windows Error Reporting's own records in the Application event log (event 1000 = crash, 1002 = hang)
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

class ExitWatch {
public:
    ~ExitWatch() { Clear(); }
    void Track(DWORD pid);          // call for each game process while the session runs
    void Clear();
    // After the game closed: "" (normal / unknown), "crash 0xC0000005", "crash" or "hang"
    std::string Result(const std::vector<std::string>& exes, uint64_t sessionMs);

    static std::string FromEventLog(const std::vector<std::string>& exes, uint64_t withinMs);   // "crash" / "hang" / ""
    static bool IsCrashCode(DWORD code) { return code >= 0xC0000000 && code != 0xC000013A; }   // not Ctrl+C / console close

private:
    std::map<DWORD, HANDLE> handles_;
};
