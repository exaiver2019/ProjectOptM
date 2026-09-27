// Stutter-cause finder: while a game runs, a Windows kernel trace records how long each driver's
// interrupt work (DPCs and ISRs) takes - the same data LatencyMon and xperf use. When the game stutters,
// the driver that held a CPU core for longest during that frame gets the blame.
// Nothing touches the game; it needs admin, and costs a little CPU while on (it's opt-in).
#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <windows.h>

class LatencyTrace {
public:
    ~LatencyTrace() { Stop(); }
    bool Start();                      // false + LastError() on failure
    void Stop();
    bool Running() const { return running_; }
    std::string LastError() const;

    // Main thread: hand over the stutters seen since the last call (QPC start, end)
    void AddStutters(const std::vector<std::pair<int64_t, int64_t>>& spans);

    struct Driver {
        std::string name;              // "ndis.sys"
        double totalMs = 0, maxUs = 0;
        uint64_t count = 0, over1ms = 0;
        int stutters = 0;              // stutters it was the longest driver in
    };
    std::vector<Driver> Report();      // busiest first (by stutters, then longest single delay)
    int StuttersSeen() const { return stuttersSeen_; }
    int StuttersExplained() const { return explained_; }

    // Called from the trace thread
    void OnEvent(int64_t ts, bool isr, uint64_t initial, uint64_t routine);

private:
    void Consume();
    void Match();                      // stutters against recent long DPCs / ISRs
    std::string DriverOf(uint64_t addr);

    std::atomic<bool> running_{ false };
    uint64_t session_ = 0, trace_ = 0;
    std::thread thread_;
    int64_t qpcFreq_ = 1;

    mutable std::mutex mu_;
    std::string error_;
    struct Agg { double totalMs = 0, maxUs = 0; uint64_t count = 0, over1ms = 0; };
    std::unordered_map<uint64_t, Agg> byRoutine_;
    struct Long { int64_t start, end; uint64_t routine; };
    std::vector<Long> recent_;                          // DPCs / ISRs of 100 us or more, oldest first
    std::vector<std::pair<int64_t, int64_t>> waiting_;  // stutters not matched yet
    std::map<uint64_t, int> blame_;                     // routine -> stutters
    int stuttersSeen_ = 0, explained_ = 0;

    std::vector<std::pair<uint64_t, std::string>> drivers_;   // loaded drivers: base address, name
};
