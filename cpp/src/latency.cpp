#include "latency.h"
#include "processes.h"
#include "util.h"
#include <algorithm>
#include <evntcons.h>
#include <evntrace.h>
#include <psapi.h>

namespace {

const wchar_t* kSession = L"ProjectOptM-Latency";
// our own id for the session (system logger sessions need one that isn't the kernel logger's)
const GUID kSessionGuid = { 0x6f1d1a0e, 0x2c1b, 0x4b8e, { 0x9a, 0x51, 0x4f, 0x70, 0x74, 0x4d, 0x4c, 0x01 } };
// PerfInfo kernel events: DPC 66, ISR 67, timer DPC 68, threaded DPC 69
const GUID kPerfInfo = { 0xce1dbfb4, 0x137e, 0x4da6, { 0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc } };

struct Props {
    EVENT_TRACE_PROPERTIES p;
    wchar_t name[64];
};

void InitProps(Props& pr) {
    ZeroMemory(&pr, sizeof(pr));
    pr.p.Wnode.BufferSize = sizeof(pr);
    pr.p.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pr.p.Wnode.ClientContext = 1;                 // QueryPerformanceCounter timestamps, like the frame capture
    pr.p.Wnode.Guid = kSessionGuid;
    pr.p.LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
    pr.p.EnableFlags = EVENT_TRACE_FLAG_DPC | EVENT_TRACE_FLAG_INTERRUPT;
    pr.p.BufferSize = 64;                         // KB - DPCs are frequent
    pr.p.MinimumBuffers = 16;
    pr.p.MaximumBuffers = 128;
    pr.p.FlushTimer = 1;
    pr.p.LoggerNameOffset = offsetof(Props, name);
}

void WINAPI OnRecord(PEVENT_RECORD r) {
    const EVENT_HEADER& h = r->EventHeader;
    if (!IsEqualGUID(h.ProviderId, kPerfInfo)) return;
    UCHAR op = h.EventDescriptor.Opcode;
    if (op < 66 || op > 69) return;
    bool is32 = (h.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) != 0;
    size_t ptr = is32 ? 4 : 8;
    if (r->UserDataLength < 8 + ptr) return;
    const BYTE* d = (const BYTE*)r->UserData;
    uint64_t initial = *(const uint64_t*)d;
    uint64_t routine = is32 ? *(const uint32_t*)(d + 8) : *(const uint64_t*)(d + 8);
    ((LatencyTrace*)r->UserContext)->OnEvent(h.TimeStamp.QuadPart, op == 67, initial, routine);
}

}  // namespace

bool LatencyTrace::Start() {
    Stop();
    {
        std::lock_guard<std::mutex> l(mu_);
        error_.clear(); byRoutine_.clear(); recent_.clear(); waiting_.clear(); blame_.clear();
        stuttersSeen_ = explained_ = 0;
    }
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); qpcFreq_ = f.QuadPart;
    proc::EnablePrivilege(L"SeSystemProfilePrivilege");
    proc::EnablePrivilege(L"SeDebugPrivilege");   // Windows 11 24H2 only shows driver addresses with it

    // the loaded drivers, to name the code behind each delay
    drivers_.clear();
    std::vector<LPVOID> bases(2048);
    DWORD need = 0;
    if (EnumDeviceDrivers(bases.data(), (DWORD)(bases.size() * sizeof(LPVOID)), &need)) {
        size_t n = std::min(bases.size(), (size_t)(need / sizeof(LPVOID)));
        for (size_t i = 0; i < n; i++) {
            wchar_t name[MAX_PATH] = {};
            if (bases[i] && GetDeviceDriverBaseNameW(bases[i], name, MAX_PATH)) drivers_.push_back({ (uint64_t)bases[i], util::Narrow(name) });
        }
        std::sort(drivers_.begin(), drivers_.end());
    }
    if (drivers_.empty()) { std::lock_guard<std::mutex> l(mu_); error_ = "Windows didn't list the loaded drivers (needs admin)"; return false; }

    Props pr; InitProps(pr);
    TRACEHANDLE session = 0;
    ULONG rc = StartTraceW(&session, kSession, &pr.p);
    if (rc == ERROR_ALREADY_EXISTS) {             // left over from a crash
        Props stop; InitProps(stop);
        ControlTraceW(0, kSession, &stop.p, EVENT_TRACE_CONTROL_STOP);
        InitProps(pr);
        rc = StartTraceW(&session, kSession, &pr.p);
    }
    if (rc != ERROR_SUCCESS) {
        std::lock_guard<std::mutex> l(mu_);
        error_ = rc == ERROR_ACCESS_DENIED ? "needs admin rights" : "couldn't start the driver trace (error " + std::to_string(rc) + ")";
        return false;
    }
    session_ = session;
    EVENT_TRACE_LOGFILEW lf = {};
    lf.LoggerName = (LPWSTR)kSession;
    lf.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    lf.EventRecordCallback = OnRecord;
    lf.Context = this;
    TRACEHANDLE t = OpenTraceW(&lf);
    if (t == INVALID_PROCESSTRACE_HANDLE) { { std::lock_guard<std::mutex> l(mu_); error_ = "couldn't open the driver trace"; } Stop(); return false; }
    trace_ = t;
    running_ = true;
    thread_ = std::thread([this] { Consume(); });
    return true;
}

void LatencyTrace::Consume() {
    TRACEHANDLE t = trace_;
    ULONG rc = ProcessTrace(&t, 1, nullptr, nullptr);
    if (running_ && rc != ERROR_SUCCESS && rc != ERROR_CANCELLED) {
        std::lock_guard<std::mutex> l(mu_);
        error_ = "driver trace stopped (error " + std::to_string(rc) + ")";
    }
    running_ = false;
}

void LatencyTrace::Stop() {
    running_ = false;
    if (session_) {
        Props pr; InitProps(pr);
        ControlTraceW(session_, nullptr, &pr.p, EVENT_TRACE_CONTROL_STOP);
        session_ = 0;
    }
    if (trace_) { CloseTrace(trace_); trace_ = 0; }
    if (thread_.joinable()) thread_.join();
}

std::string LatencyTrace::LastError() const {
    std::lock_guard<std::mutex> l(mu_);
    return error_;
}

void LatencyTrace::OnEvent(int64_t ts, bool, uint64_t initial, uint64_t routine) {
    int64_t dur = ts - (int64_t)initial;
    if (dur < 0 || dur > qpcFreq_) return;   // not a sane duration (over a second)
    double us = dur * 1e6 / qpcFreq_;
    std::lock_guard<std::mutex> l(mu_);
    Agg& a = byRoutine_[routine];
    a.totalMs += us / 1000;
    a.count++;
    if (us > a.maxUs) a.maxUs = us;
    if (us >= 1000) a.over1ms++;
    if (us >= 100) {
        recent_.push_back({ (int64_t)initial, ts, routine });
        // keep ~10 s
        if (recent_.size() > 256 && recent_.front().end < ts - qpcFreq_ * 10)
            recent_.erase(recent_.begin(), std::find_if(recent_.begin(), recent_.end(), [&](const Long& x) { return x.end >= ts - qpcFreq_ * 10; }));
        if (recent_.size() > 200000) recent_.erase(recent_.begin(), recent_.begin() + 100000);
    }
}

void LatencyTrace::AddStutters(const std::vector<std::pair<int64_t, int64_t>>& spans) {
    std::lock_guard<std::mutex> l(mu_);
    stuttersSeen_ += (int)spans.size();
    waiting_.insert(waiting_.end(), spans.begin(), spans.end());
    Match();
}

// Runs under mu_. A stutter is matched once the trace has delivered events past its end
// (the trace flushes about once a second).
void LatencyTrace::Match() {
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    std::vector<std::pair<int64_t, int64_t>> later;
    for (auto& s : waiting_) {
        if (s.second > now.QuadPart - qpcFreq_ * 2) { later.push_back(s); continue; }
        const Long* worst = nullptr;
        for (auto& e : recent_)
            if (e.end >= s.first && e.start <= s.second && (!worst || e.end - e.start > worst->end - worst->start)) worst = &e;
        // half a millisecond of one core stuck in a driver is enough to push a frame late
        if (worst && (worst->end - worst->start) * 1e6 / qpcFreq_ >= 500) { blame_[worst->routine]++; explained_++; }
    }
    waiting_ = later;
}

std::string LatencyTrace::DriverOf(uint64_t addr) {
    auto it = std::upper_bound(drivers_.begin(), drivers_.end(), std::make_pair(addr, std::string("\xff")));
    if (it == drivers_.begin()) return "unknown";
    return std::prev(it)->second;
}

std::vector<LatencyTrace::Driver> LatencyTrace::Report() {
    std::map<std::string, Driver> by;
    {
        std::lock_guard<std::mutex> l(mu_);
        Match();
        for (auto& [routine, a] : byRoutine_) {
            Driver& d = by[DriverOf(routine)];
            d.totalMs += a.totalMs;
            d.count += a.count;
            d.over1ms += a.over1ms;
            d.maxUs = std::max(d.maxUs, a.maxUs);
        }
        for (auto& [routine, n] : blame_) by[DriverOf(routine)].stutters += n;
    }
    std::vector<Driver> out;
    for (auto& [name, d] : by) { Driver x = d; x.name = name; out.push_back(x); }
    std::sort(out.begin(), out.end(), [](const Driver& a, const Driver& b) {
        return a.stutters != b.stutters ? a.stutters > b.stutters : a.maxUs > b.maxUs;
    });
    return out;
}
