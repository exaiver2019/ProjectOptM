// Live frame timing, captured straight from Windows (ETW present events from DXGI and
// Direct3D 9 - the same data PresentMon reads), plus the FPS statistics built on it.
#pragma once
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
    void ResetSession();
    bool SessionStats(double& avgFps, double& low1) const;          // whole session; false if too short

    // Called from the ETW thread
    void OnPresent(DWORD pid, uint64_t swapChain, int64_t timestamp);

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
    std::vector<double> pending_;
    std::string error_;

    std::vector<double> buffer_;
    std::vector<uint32_t> hist_ = std::vector<uint32_t>(4001);   // 0.05 ms buckets
    double sessionSum_ = 0;
    uint64_t sessionN_ = 0;
};

namespace frames {
struct Live { double fps = 0, low1 = 0, frametime = 0; };
Live Stats(const std::vector<double>& ft);                                     // fps (1s), 1% low (10s), avg frametime (1s)
std::vector<double> Columns(const std::vector<double>& ft, double windowMs, int cols);   // worst frametime per column
}
