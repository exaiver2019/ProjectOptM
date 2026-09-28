#include "frames.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <evntrace.h>
#include <evntcons.h>

namespace {

const wchar_t* kSession = L"ProjectOptM";

// Microsoft-Windows-DXGI: Present_Start = 42, PresentMultiplaneOverlay_Start = 55
const GUID kDxgi = { 0xCA11C036, 0x0102, 0x4A2D, { 0xA6, 0xAD, 0xF0, 0x3C, 0xFE, 0xD5, 0xD3, 0xC9 } };
// Microsoft-Windows-D3D9: Present_Start = 1
const GUID kD3d9 = { 0x783ACA0A, 0x790E, 0x4D7F, { 0x84, 0x51, 0xAA, 0x85, 0x05, 0x11, 0xC6, 0xB9 } };

struct Props {
    EVENT_TRACE_PROPERTIES p;
    wchar_t name[64];
};

void InitProps(Props& pr) {
    ZeroMemory(&pr, sizeof(pr));
    pr.p.Wnode.BufferSize = sizeof(pr);
    pr.p.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pr.p.Wnode.ClientContext = 1;                 // QueryPerformanceCounter timestamps
    pr.p.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pr.p.BufferSize = 16;                         // KB
    pr.p.MinimumBuffers = 8;
    pr.p.MaximumBuffers = 64;
    pr.p.FlushTimer = 1;                          // seconds - keeps the graph close to live
    pr.p.LoggerNameOffset = offsetof(Props, name);
}

bool Enable(TRACEHANDLE session, const GUID& provider, std::vector<USHORT> ids) {
    std::vector<BYTE> buf(sizeof(EVENT_FILTER_EVENT_ID) + ids.size() * sizeof(USHORT));
    auto* f = (EVENT_FILTER_EVENT_ID*)buf.data();
    f->FilterIn = TRUE;
    f->Count = (USHORT)ids.size();
    for (size_t i = 0; i < ids.size(); i++) f->Events[i] = ids[i];
    EVENT_FILTER_DESCRIPTOR desc = {};
    desc.Ptr = (ULONGLONG)f;
    desc.Size = (ULONG)(sizeof(EVENT_FILTER_EVENT_ID) + (ids.size() - 1) * sizeof(USHORT));
    desc.Type = EVENT_FILTER_TYPE_EVENT_ID;
    ENABLE_TRACE_PARAMETERS params = {};
    params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.EnableFilterDesc = &desc;
    params.FilterDescCount = 1;
    return EnableTraceEx2(session, &provider, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION,
                          ~0ull, 0, 0, &params) == ERROR_SUCCESS;
}

void WINAPI OnEvent(PEVENT_RECORD r) {
    auto* self = (FrameCapture*)r->UserContext;
    const EVENT_HEADER& h = r->EventHeader;
    USHORT id = h.EventDescriptor.Id;
    bool dxgi = IsEqualGUID(h.ProviderId, kDxgi) && (id == 42 || id == 55);
    bool d3d9 = IsEqualGUID(h.ProviderId, kD3d9) && id == 1;
    if (!dxgi && !d3d9) return;
    // first field is the swap chain pointer (4 or 8 bytes, depending on the game)
    uint64_t chain = 0;
    bool is32 = (h.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) != 0;
    if (r->UserDataLength >= (is32 ? 4 : 8)) chain = is32 ? *(const uint32_t*)r->UserData : *(const uint64_t*)r->UserData;
    self->OnPresent(h.ProcessId, chain, h.TimeStamp.QuadPart);
}

}  // namespace

bool FrameCapture::Start(const std::vector<DWORD>& pids) {
    Stop();
    {
        std::lock_guard<std::mutex> l(mu_);
        pids_ = std::set<DWORD>(pids.begin(), pids.end());
        last_.clear(); counts_.clear(); top_ = { 0, 0 }; pending_.clear(); error_.clear(); blocked_ = false;
    }
    buffer_.clear();
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); qpcFreq_ = f.QuadPart;

    Props pr; InitProps(pr);
    TRACEHANDLE session = 0;
    ULONG rc = StartTraceW(&session, kSession, &pr.p);
    if (rc == ERROR_ALREADY_EXISTS) {             // left over from a crash (or 1.1's PresentMon session)
        Props stop; InitProps(stop);
        ControlTraceW(0, kSession, &stop.p, EVENT_TRACE_CONTROL_STOP);
        InitProps(pr);
        rc = StartTraceW(&session, kSession, &pr.p);
    }
    if (rc != ERROR_SUCCESS) {
        std::lock_guard<std::mutex> l(mu_);
        if (rc != ERROR_ACCESS_DENIED) error_ = "couldn't start the trace (error " + std::to_string(rc) + ")";
        else if (!util::IsElevated()) error_ = "needs admin rights";
        else { error_ = "Windows refused it - usually the game's anti-cheat blocking frame capture"; blocked_ = true; }
        return false;
    }
    session_ = session;
    bool ok = Enable(session, kDxgi, { 42, 55 });
    ok = Enable(session, kD3d9, { 1 }) || ok;
    if (!ok) { std::lock_guard<std::mutex> l(mu_); error_ = "couldn't enable the Direct3D providers"; Stop(); return false; }

    EVENT_TRACE_LOGFILEW lf = {};
    lf.LoggerName = (LPWSTR)kSession;
    lf.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    lf.EventRecordCallback = OnEvent;
    lf.Context = this;
    TRACEHANDLE t = OpenTraceW(&lf);
    if (t == INVALID_PROCESSTRACE_HANDLE) { std::lock_guard<std::mutex> l(mu_); error_ = "couldn't open the trace"; Stop(); return false; }
    trace_ = t;
    running_ = true;
    thread_ = std::thread([this] { Consume(); });
    return true;
}

void FrameCapture::Consume() {
    TRACEHANDLE t = trace_;
    ULONG rc = ProcessTrace(&t, 1, nullptr, nullptr);
    if (running_ && rc != ERROR_SUCCESS && rc != ERROR_CANCELLED) {
        std::lock_guard<std::mutex> l(mu_);
        error_ = "capture stopped (error " + std::to_string(rc) + ")";
    }
    running_ = false;
}

void FrameCapture::Stop() {
    running_ = false;
    if (session_) {
        Props pr; InitProps(pr);
        ControlTraceW(session_, nullptr, &pr.p, EVENT_TRACE_CONTROL_STOP);
        session_ = 0;
    }
    if (trace_) { CloseTrace(trace_); trace_ = 0; }
    if (thread_.joinable()) thread_.join();
}

void FrameCapture::SetPids(const std::vector<DWORD>& pids) {
    std::lock_guard<std::mutex> l(mu_);
    pids_ = std::set<DWORD>(pids.begin(), pids.end());
}

std::string FrameCapture::LastError() const {
    std::lock_guard<std::mutex> l(mu_);
    return error_;
}

void FrameCapture::OnPresent(DWORD pid, uint64_t swapChain, int64_t ts) {
    std::lock_guard<std::mutex> l(mu_);
    if (!pids_.count(pid)) return;
    auto key = std::make_pair(pid, swapChain);
    auto it = last_.find(key);
    int64_t prev = it == last_.end() ? 0 : it->second;
    last_[key] = ts;
    if (!prev) return;
    double ms = (ts - prev) * 1000.0 / qpcFreq_;
    if (ms <= 0 || ms > 2000) return;
    // a game can have several swap chains (launchers, overlays): follow the busiest one
    int n = ++counts_[key];
    if (top_.first == 0 || n > counts_[top_]) top_ = key;
    if (key != top_) return;
    pending_.push_back({ ms, ts });
}

void FrameCapture::Pump() {
    std::vector<Frame> fresh;
    {
        std::lock_guard<std::mutex> l(mu_);
        fresh.swap(pending_);
    }
    if (fresh.empty()) return;
    std::vector<double> sorted;
    for (const Frame& f : fresh) {
        double v = f.ms;
        int b = std::min(4000, (int)(v / 0.05));
        allHist_[b]++;
        sessionSum_ += v;
        sessionN_++;
        if (!loading_) { hist_[b]++; playN_++; }   // the lows come from gameplay frames only
        // stutter: compared with the median of the last 90 frames (needs 30 to judge)
        bool st = false;
        if (recent_.size() >= 30) {
            sorted = recent_;
            std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
            st = IsStutter(v, sorted[sorted.size() / 2]);
        }
        if (st) {
            stutters_++;
            if (loading_) loadStutters_++;
            spans_.push_back({ f.ts - (int64_t)(v * qpcFreq_ / 1000.0), f.ts });
            if (spans_.size() > 5000) spans_.erase(spans_.begin());
        }
        recent_.push_back(v);
        if (recent_.size() > 90) recent_.erase(recent_.begin());
        buffer_.push_back(v);
        flags_.push_back(!st ? 0 : loading_ ? 2 : 1);
    }
    if (buffer_.size() > 20000) {
        buffer_.erase(buffer_.begin(), buffer_.end() - 20000);
        flags_.erase(flags_.begin(), flags_.end() - 20000);
    }
}

void FrameCapture::ResetSession() {
    std::fill(hist_.begin(), hist_.end(), 0);
    std::fill(allHist_.begin(), allHist_.end(), 0);
    sessionSum_ = 0;
    sessionN_ = playN_ = 0;
    stutters_ = loadStutters_ = 0;
    loading_ = false;
    buffer_.clear();
    flags_.clear();
    recent_.clear();
    spans_.clear();
}

std::vector<std::pair<int64_t, int64_t>> FrameCapture::TakeStutterSpans() {
    std::vector<std::pair<int64_t, int64_t>> out;
    out.swap(spans_);
    return out;
}

void FrameCapture::InjectForTest(double ms) {
    if (qpcFreq_ <= 1) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); qpcFreq_ = f.QuadPart; }
    if (!testTs_) { LARGE_INTEGER c; QueryPerformanceCounter(&c); testTs_ = c.QuadPart; }
    testTs_ += (int64_t)(ms * qpcFreq_ / 1000.0);
    std::lock_guard<std::mutex> l(mu_);
    pending_.push_back({ ms, testTs_ });
}

// FPS the slowest `frac` of frames fall under - from gameplay frames when there are enough of them
// (a session that was mostly loading falls back to every frame)
double FrameCapture::LowFrom(double frac, uint64_t minFrames) const {
    // a game that "loads" most of the time just streams from disk while you play: then every frame counts
    bool play = playN_ >= minFrames && playN_ * 2 >= sessionN_;
    const std::vector<uint32_t>& h = play ? hist_ : allHist_;
    uint64_t n = play ? playN_ : sessionN_;
    if (n < minFrames) return 0;
    uint64_t need = (uint64_t)std::ceil(n * frac), acc = 0;
    for (int b = 4000; b >= 0; b--) {
        acc += h[b];
        if (acc >= need) return 1000.0 / ((b + 0.5) * 0.05);
    }
    return 0;
}

double FrameCapture::SessionLowPct(double frac) const { return LowFrom(frac, 100); }

double FrameCapture::SessionLow01() const { return LowFrom(0.001, 1000); }   // under 1000 frames the 0.1% is one frame - too noisy

bool FrameCapture::SessionStats(double& avgFps, double& low1) const {
    avgFps = low1 = 0;
    if (sessionN_ < 100 || sessionSum_ <= 0) return false;
    avgFps = sessionN_ * 1000.0 / sessionSum_;
    low1 = LowFrom(0.01, 100);
    return true;
}

namespace frames {

Live Stats(const std::vector<double>& ft) {
    Live s;
    double sum = 0; int cnt = 0;
    for (int i = (int)ft.size() - 1; i >= 0 && sum < 1000; i--) { sum += ft[i]; cnt++; }
    s.fps = sum > 0 ? cnt * 1000.0 / sum : 0;
    s.frametime = cnt ? sum / cnt : 0;
    std::vector<double> w;
    double s2 = 0;
    for (int i = (int)ft.size() - 1; i >= 0 && s2 < 10000; i--) { s2 += ft[i]; w.push_back(ft[i]); }
    if (w.size() >= 50) {
        std::sort(w.begin(), w.end());
        int idx = std::max(0, (int)std::ceil(w.size() * 0.99) - 1);
        s.low1 = 1000.0 / w[idx];
    }
    return s;
}

std::vector<double> Columns(const std::vector<double>& ft, double windowMs, int cols) {
    std::vector<double> c(cols, 0.0);
    double t = 0;
    for (int i = (int)ft.size() - 1; i >= 0; i--) {
        t += ft[i];
        if (t > windowMs) break;
        int col = std::max(0, cols - 1 - (int)(t / windowMs * cols));
        c[col] = std::max(c[col], ft[i]);
    }
    for (int i = 1; i < cols; i++) if (c[i] == 0) c[i] = c[i - 1];
    return c;
}

std::vector<uint8_t> StutterColumns(const std::vector<double>& ft, const std::vector<uint8_t>& flags, double windowMs, int cols) {
    std::vector<uint8_t> c(cols, 0);
    if (flags.size() != ft.size()) return c;
    double t = 0;
    for (int i = (int)ft.size() - 1; i >= 0; i--) {
        t += ft[i];
        if (t > windowMs) break;
        if (flags[i] == 1) c[std::max(0, cols - 1 - (int)(t / windowMs * cols))] = 1;   // gameplay stutters (2 = while loading)
    }
    return c;
}

int StuttersIn(const std::vector<double>& ft, const std::vector<uint8_t>& flags, double windowMs) {
    if (flags.size() != ft.size()) return 0;
    double t = 0;
    int n = 0;
    for (int i = (int)ft.size() - 1; i >= 0 && t < windowMs; i--) { t += ft[i]; n += flags[i] == 1; }
    return n;
}

}  // namespace frames
