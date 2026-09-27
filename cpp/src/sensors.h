// PC readings for the overlay: GPU usage, VRAM, GPU temperature, CPU usage, RAM.
// Read once a second on a background thread, from the same sources Task Manager uses
// (Windows performance counters and the graphics driver) - nothing touches the game.
#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

struct Readings {
    double gpuPct = -1;          // -1 = not available
    double gpuTempC = -1;
    double vramUsedGB = -1, vramTotalGB = -1;
    double cpuPct = -1;
    double ramUsedGB = -1, ramTotalGB = -1;
};

class Sensors {
public:
    ~Sensors() { Stop(); }
    void Start(uint32_t luidLow, int32_t luidHigh, uint64_t vramBytes);   // the GPU to watch
    void Stop();
    void SetActive(bool on) { active_ = on; }   // only read while something shows the numbers
    Readings Get() const;

private:
    void Run();
    std::thread thread_;
    std::atomic<bool> stop_{ false }, active_{ false };
    uint32_t luidLow_ = 0;
    int32_t luidHigh_ = 0;
    uint64_t vramBytes_ = 0;
    mutable std::mutex mu_;
    Readings r_;
};
