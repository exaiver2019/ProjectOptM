#include "netping.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <evntcons.h>
#include <evntrace.h>
#include <algorithm>

namespace {

const wchar_t* kSession = L"ProjectOptM-Net";
// Microsoft-Windows-Kernel-Network: TCP send IPv4 = 10, IPv6 = 26; UDP send IPv4 = 42, IPv6 = 58
const GUID kKernelNetwork = { 0x7DD42A49, 0x5329, 0x4832, { 0x8D, 0xFD, 0x43, 0xD9, 0x79, 0x15, 0x3A, 0x88 } };

struct Props {
    EVENT_TRACE_PROPERTIES p;
    wchar_t name[64];
};

void InitProps(Props& pr) {
    ZeroMemory(&pr, sizeof(pr));
    pr.p.Wnode.BufferSize = sizeof(pr);
    pr.p.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pr.p.Wnode.ClientContext = 1;
    pr.p.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pr.p.BufferSize = 16;
    pr.p.MinimumBuffers = 4;
    pr.p.MaximumBuffers = 32;
    pr.p.FlushTimer = 1;
    pr.p.LoggerNameOffset = offsetof(Props, name);
}

void WINAPI OnRecord(PEVENT_RECORD r) {
    const EVENT_HEADER& h = r->EventHeader;
    if (!IsEqualGUID(h.ProviderId, kKernelNetwork)) return;
    USHORT id = h.EventDescriptor.Id;
    bool v6 = id == 26 || id == 58, udp = id == 42 || id == 58;
    if (id != 10 && id != 26 && id != 42 && id != 58) return;
    // PID, size, daddr, saddr, dport, sport, ...
    int alen = v6 ? 16 : 4;
    if (r->UserDataLength < 8 + alen * 2 + 4) return;
    const BYTE* d = (const BYTE*)r->UserData;
    DWORD pid = *(const uint32_t*)d;
    uint32_t size = *(const uint32_t*)(d + 4);
    uint16_t port = ntohs(*(const uint16_t*)(d + 8 + alen * 2));
    ((NetPing*)r->UserContext)->OnSend(pid, udp, d + 8, alen, port, size);
}

bool Interesting(const BYTE* a, int len, uint16_t port) {
    if (port == 53 || port == 5353 || port == 1900) return false;   // name lookups, discovery
    if (len == 4) {
        if (a[0] == 127 || a[0] == 0 || a[0] >= 224) return false;           // loopback, multicast
        if (a[0] == 169 && a[1] == 254) return false;                        // link-local
        return true;
    }
    static const BYTE zero[16] = {};
    if (!memcmp(a, zero, 15) && a[15] <= 1) return false;                    // :: and ::1
    if (a[0] == 0xFE && (a[1] & 0xC0) == 0x80) return false;                 // link-local
    if (a[0] == 0xFF) return false;                                          // multicast
    return true;
}

}  // namespace

bool NetPing::Start(const std::vector<DWORD>& pids) {
    Stop();
    {
        std::lock_guard<std::mutex> l(mu_);
        pids_ = std::set<DWORD>(pids.begin(), pids.end());
        traffic_.clear(); server_.clear(); serverAddr_.clear(); error_.clear();
        last_ = -1; sum_ = 0; replies_ = misses_ = 0;
    }
    Props pr; InitProps(pr);
    TRACEHANDLE session = 0;
    ULONG rc = StartTraceW(&session, kSession, &pr.p);
    if (rc == ERROR_ALREADY_EXISTS) {
        Props stop; InitProps(stop);
        ControlTraceW(0, kSession, &stop.p, EVENT_TRACE_CONTROL_STOP);
        InitProps(pr);
        rc = StartTraceW(&session, kSession, &pr.p);
    }
    if (rc != ERROR_SUCCESS) {
        std::lock_guard<std::mutex> l(mu_);
        error_ = rc == ERROR_ACCESS_DENIED ? "needs admin rights" : "couldn't start the network trace (error " + std::to_string(rc) + ")";
        return false;
    }
    session_ = session;
    USHORT ids[] = { 10, 26, 42, 58 };
    std::vector<BYTE> buf(sizeof(EVENT_FILTER_EVENT_ID) + 4 * sizeof(USHORT));
    auto* f = (EVENT_FILTER_EVENT_ID*)buf.data();
    f->FilterIn = TRUE;
    f->Count = 4;
    for (int i = 0; i < 4; i++) f->Events[i] = ids[i];
    EVENT_FILTER_DESCRIPTOR desc = {};
    desc.Ptr = (ULONGLONG)f;
    desc.Size = (ULONG)(sizeof(EVENT_FILTER_EVENT_ID) + 3 * sizeof(USHORT));
    desc.Type = EVENT_FILTER_TYPE_EVENT_ID;
    ENABLE_TRACE_PARAMETERS params = {};
    params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.EnableFilterDesc = &desc;
    params.FilterDescCount = 1;
    ULONG en = EnableTraceEx2(session, &kKernelNetwork, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, ~0ull, 0, 0, &params);
    if (en != ERROR_SUCCESS) {
        { std::lock_guard<std::mutex> l(mu_); error_ = en == ERROR_ACCESS_DENIED ? "needs admin rights" : "couldn't enable the network events (error " + std::to_string(en) + ")"; }
        Stop();
        return false;
    }
    EVENT_TRACE_LOGFILEW lf = {};
    lf.LoggerName = (LPWSTR)kSession;
    lf.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    lf.EventRecordCallback = OnRecord;
    lf.Context = this;
    TRACEHANDLE t = OpenTraceW(&lf);
    if (t == INVALID_PROCESSTRACE_HANDLE) { { std::lock_guard<std::mutex> l(mu_); error_ = "couldn't open the network trace"; } Stop(); return false; }
    trace_ = t;
    running_ = true;
    stop_ = false;
    thread_ = std::thread([this] { Consume(); });
    pinger_ = std::thread([this] { PingLoop(); });
    return true;
}

void NetPing::Consume() {
    TRACEHANDLE t = trace_;
    ProcessTrace(&t, 1, nullptr, nullptr);
    running_ = false;
}

void NetPing::Stop() {
    stop_ = true;
    running_ = false;
    if (session_) {
        Props pr; InitProps(pr);
        ControlTraceW(session_, nullptr, &pr.p, EVENT_TRACE_CONTROL_STOP);
        session_ = 0;
    }
    if (trace_) { CloseTrace(trace_); trace_ = 0; }
    if (thread_.joinable()) thread_.join();
    if (pinger_.joinable()) pinger_.join();
}

void NetPing::SetPids(const std::vector<DWORD>& pids) {
    std::lock_guard<std::mutex> l(mu_);
    pids_ = std::set<DWORD>(pids.begin(), pids.end());
}

std::string NetPing::LastError() const { std::lock_guard<std::mutex> l(mu_); return error_; }
double NetPing::PingMs() const { std::lock_guard<std::mutex> l(mu_); return last_; }
double NetPing::SessionAvg() const { std::lock_guard<std::mutex> l(mu_); return replies_ ? sum_ / replies_ : -1; }
std::string NetPing::Server() const { std::lock_guard<std::mutex> l(mu_); return server_; }
bool NetPing::NoReply() const { std::lock_guard<std::mutex> l(mu_); return !server_.empty() && replies_ == 0 && misses_ >= 3; }

void NetPing::OnSend(DWORD pid, bool udp, const BYTE* addr, int len, uint16_t port, uint32_t bytes) {
    if (!Interesting(addr, len, port)) return;
    std::lock_guard<std::mutex> l(mu_);
    if (!pids_.count(pid)) return;
    char text[64] = {};
    inet_ntop(len == 4 ? AF_INET : AF_INET6, addr, text, sizeof(text));
    Traffic& t = traffic_[text];
    if (t.addr.empty()) t.addr.assign(addr, addr + len);
    // web traffic (store pages, telemetry) isn't the game server
    if (udp) t.udp += bytes; else if (port != 443 && port != 80) t.tcp += bytes;
    t.at = GetTickCount64();
}

void NetPing::PingLoop() {
    HANDLE icmp4 = IcmpCreateFile(), icmp6 = Icmp6CreateFile();
    uint64_t lastDecay = GetTickCount64();
    while (!stop_) {
        for (int i = 0; i < 20 && !stop_; i++) Sleep(100);   // every 2 s
        if (stop_) break;
        std::vector<BYTE> target;
        {
            std::lock_guard<std::mutex> l(mu_);
            // the server: most UDP bytes (games talk UDP), else most non-web TCP
            std::string best; uint64_t bestN = 0; bool bestUdp = false;
            for (auto& [a, t] : traffic_) {
                bool u = t.udp > 0;
                uint64_t n = u ? t.udp : t.tcp;
                if (!n) continue;
                if ((u && !bestUdp) || ((u == bestUdp) && n > bestN)) { best = a; bestN = n; bestUdp = u; }
            }
            if (!best.empty() && best != server_) { server_ = best; serverAddr_ = traffic_[best].addr; replies_ = misses_ = 0; sum_ = 0; last_ = -1; }
            target = serverAddr_;
            if (GetTickCount64() - lastDecay > 10000) {   // older traffic counts less, so a server change is picked up
                lastDecay = GetTickCount64();
                for (auto it = traffic_.begin(); it != traffic_.end();) {
                    it->second.udp /= 2; it->second.tcp /= 2;
                    if (!it->second.udp && !it->second.tcp) it = traffic_.erase(it); else ++it;
                }
            }
        }
        if (target.empty()) continue;
        double ms = -1;
        char reply[256];
        char data[8] = "OptM";
        if (target.size() == 4 && icmp4 != INVALID_HANDLE_VALUE) {
            IPAddr ip; memcpy(&ip, target.data(), 4);
            if (IcmpSendEcho(icmp4, ip, data, sizeof(data), nullptr, reply, sizeof(reply), 1000) > 0) {
                auto* e = (ICMP_ECHO_REPLY*)reply;
                if (e->Status == IP_SUCCESS) ms = std::max<ULONG>(e->RoundTripTime, 1);
            }
        } else if (target.size() == 16 && icmp6 != INVALID_HANDLE_VALUE) {
            sockaddr_in6 src = {}, dst = {};
            src.sin6_family = dst.sin6_family = AF_INET6;
            memcpy(&dst.sin6_addr, target.data(), 16);
            if (Icmp6SendEcho2(icmp6, nullptr, nullptr, nullptr, &src, &dst, data, sizeof(data), nullptr, reply, sizeof(reply), 1000) > 0) {
                auto* e = (ICMPV6_ECHO_REPLY*)reply;
                if (e->Status == IP_SUCCESS) ms = std::max<ULONG>(e->RoundTripTime, 1);
            }
        }
        std::lock_guard<std::mutex> l(mu_);
        if (ms >= 0) { last_ = ms; sum_ += ms; replies_++; }
        else { misses_++; if (misses_ >= 3 && replies_ == 0) last_ = -1; }
    }
    if (icmp4 != INVALID_HANDLE_VALUE) IcmpCloseHandle(icmp4);
    if (icmp6 != INVALID_HANDLE_VALUE) IcmpCloseHandle(icmp6);
}
