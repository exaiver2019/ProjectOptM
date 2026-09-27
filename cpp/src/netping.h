// Game server ping: a Windows network trace shows which address the game sends most of its traffic
// to (nothing touches the game), and a normal ping to that address every 2 seconds measures the delay.
// Some servers don't answer pings - then there's no number. Needs admin (the trace).
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

class NetPing {
public:
    ~NetPing() { Stop(); }
    bool Start(const std::vector<DWORD>& pids);   // false + LastError() on failure
    void SetPids(const std::vector<DWORD>& pids);
    void Stop();
    bool Running() const { return running_; }
    std::string LastError() const;

    double PingMs() const;          // latest reply, -1 = none (yet)
    double SessionAvg() const;      // mean of the session's replies, -1 = none
    std::string Server() const;     // "203.0.113.5" ("" = not found yet)
    bool NoReply() const;           // the server is known but doesn't answer pings

    void OnSend(DWORD pid, bool udp, const BYTE* addr, int addrLen, uint16_t port, uint32_t bytes);   // trace thread

private:
    void Consume();
    void PingLoop();

    std::atomic<bool> running_{ false }, stop_{ false };
    uint64_t session_ = 0, trace_ = 0;
    std::thread thread_, pinger_;

    mutable std::mutex mu_;
    std::set<DWORD> pids_;
    std::string error_;
    struct Traffic { uint64_t udp = 0, tcp = 0; uint64_t at = 0; std::vector<BYTE> addr; };
    std::map<std::string, Traffic> traffic_;   // remote address -> bytes sent (decays)
    std::string server_;
    std::vector<BYTE> serverAddr_;
    double last_ = -1, sum_ = 0;
    int replies_ = 0, misses_ = 0;
};
