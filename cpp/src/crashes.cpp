#include "crashes.h"
#include "util.h"
#include <cstdio>
#include <winevt.h>

void ExitWatch::Track(DWORD pid) {
    if (handles_.count(pid)) return;
    HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) handles_[pid] = h;
}

void ExitWatch::Clear() {
    for (auto& [pid, h] : handles_) CloseHandle(h);
    handles_.clear();
}

std::string ExitWatch::Result(const std::vector<std::string>& exes, uint64_t sessionMs) {
    std::string out;
    for (auto& [pid, h] : handles_) {
        DWORD code = STILL_ACTIVE;
        if (WaitForSingleObject(h, 0) != WAIT_OBJECT_0 || !GetExitCodeProcess(h, &code)) continue;
        if (IsCrashCode(code)) { char b[32]; snprintf(b, sizeof(b), "crash 0x%08lX", code); out = b; }
    }
    Clear();
    // Windows writes its crash record a moment after the process is gone; the session's own time
    // window keeps an older crash of the same game out
    std::string ev = FromEventLog(exes, sessionMs + 60000);
    if (out.empty()) out = ev;
    return out;
}

std::string ExitWatch::FromEventLog(const std::vector<std::string>& exes, uint64_t withinMs) {
    if (exes.empty()) return "";
    wchar_t query[256];
    swprintf(query, 256, L"*[System[(EventID=1000 or EventID=1002) and TimeCreated[timediff(@SystemTime) <= %llu]]]",
             (unsigned long long)withinMs);
    EVT_HANDLE q = EvtQuery(nullptr, L"Application", query, EvtQueryChannelPath | EvtQueryReverseDirection);
    if (!q) return "";
    std::string found;
    EVT_HANDLE ev[16];
    DWORD got = 0;
    while (found.empty() && EvtNext(q, 16, ev, 500, 0, &got)) {
        for (DWORD i = 0; i < got; i++) {
            DWORD used = 0, props = 0;
            EvtRender(nullptr, ev[i], EvtRenderEventXml, 0, nullptr, &used, &props);
            std::wstring xml(used / sizeof(wchar_t) + 1, L'\0');
            if (found.empty() && EvtRender(nullptr, ev[i], EvtRenderEventXml, (DWORD)(xml.size() * sizeof(wchar_t)), xml.data(), &used, &props)) {
                std::string x = util::Lower(util::Narrow(xml.c_str()));
                // the first data field is the faulting app's file name: <Data Name='AppName'>game.exe</Data> (or unnamed)
                for (auto& e : exes)
                    if (x.find(">" + util::Lower(e) + ".exe<") != std::string::npos) {
                        found = x.find("<eventid>1002</eventid>") != std::string::npos || x.find(">1002<") != std::string::npos ? "hang" : "crash";
                        break;
                    }
            }
            EvtClose(ev[i]);
        }
    }
    EvtClose(q);
    return found;
}
