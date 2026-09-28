// Live frame timing, captured straight from Windows (ETW present events from DXGI and
// Direct3D 9 - the same data PresentMon reads), plus the FPS statistics built on it.
#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

class FrameCapture {
public:
    ~FrameCapture() { Stop(); }
    bool Start(const std::vector<DWORD>& pids);   // needs admin; false + LastError() on failure
    void SetPids(const std::vector<DWORD>& pids);  // game processes can come and go
    void Stop();
    bool Running() const { return running_; }
    std::string LastError() const;
    bool Blocked() const { return blocked_; }       // refused although we're admin (anti-cheat, usually)

    // Main thread, every loop: moves new frames into the rolling buffer and session stats.
    void Pump();
    const std::vector<double>& Buffer() const { return buffer_; }   // frametimes in ms, oldest first
    const std::vector<uint8_t>& Stutters() const { return flags_; } // 1 = that frame in Buffer() was a stutter
    void ResetSession();
    bool SessionStats(double& avgFps, double& low1) const;          // whole session; false if too short
    double SessionLow01() const;                                    // 0.1% low FPS (0 if too few frames)
    double SessionLowPct(double frac) const;                        // FPS the slowest frac of frames fall under (0.05 = 5% low)
    int SessionStutters() const { return stutters_; }
    double SessionSeconds() const { return sessionSum_ / 1000.0; }  // time covered by captured frames
    // stutters as QPC timestamps (start of the slow frame, end) - the stutter-cause finder looks inside them
    std::vector<std::pair<int64_t, int64_t>> TakeStutterSpans();
    void InjectForTest(double ms);                                  // developer check: a made-up frame

    // Called from the ETW thread
    void OnPresent(DWORD pid, uint64_t swapChain, int64_t timestamp);

    // A frame is a stutter when it takes much longer than the frames around it:
    // over 2.5x the recent median and at least 10 ms more (a 25 ms frame at 100 FPS, 42 ms at 60 FPS)
    static bool IsStutter(double ms, double median) { return median > 0 && ms > std::max(median * 2.5, median + 10.0); }

private:
    void Consume();

    std::atomic<bool> running_{ false }, blocked_{ false };
    uint64_t session_ = 0, trace_ = 0;        // TRACEHANDLEs
    std::thread thread_;
    int64_t qpcFreq_ = 1;

    mutable std::mutex mu_;
    std::set<DWORD> pids_;
    std::map<std::pair<DWORD, uint64_t>, int64_t> last_;   // (pid, swap chain) -> last present
    std::map<std::pair<DWORD, uint64_t>, int> counts_;
    std::pair<DWORD, uint64_t> top_{ 0, 0 };
    struct Frame { double ms; int64_t ts; };
    std::vector<Frame> pending_;
    std::string error_;

    std::vector<double> buffer_;
    std::vector<uint8_t> flags_;                                  // stutter marks, same length as buffer_
    std::vector<double> recent_;                                  // last frames, for the stutter median
    std::vector<std::pair<int64_t, int64_t>> spans_;              // stutters not yet taken
    std::vector<uint32_t> hist_ = std::vector<uint32_t>(4001);   // 0.05 ms buckets
    double sessionSum_ = 0;
    uint64_t sessionN_ = 0;
    int stutters_ = 0;
    int64_t testTs_ = 0;
};

namespace frames {
struct Live { double fps = 0, low1 = 0, frametime = 0; };
Live Stats(const std::vector<double>& ft);                                     // fps (1s), 1% low (10s), avg frametime (1s)
std::vector<double> Columns(const std::vector<double>& ft, double windowMs, int cols);   // worst frametime per column
std::vector<uint8_t> StutterColumns(const std::vector<double>& ft, const std::vector<uint8_t>& flags, double windowMs, int cols);
int StuttersIn(const std::vector<double>& ft, const std::vector<uint8_t>& flags, double windowMs);   // in the last windowMs
}
