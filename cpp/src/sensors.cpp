#include "sensors.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <algorithm>
#include <cwctype>
#include <map>
#include <string>
#include <vector>

namespace {

// ---- GPU temperature from the graphics driver (the value Task Manager shows; WDDM 2.4+ drivers)
typedef UINT KmtHandle;
struct KmtOpenFromLuid { LUID luid; KmtHandle adapter; };
struct KmtClose { KmtHandle adapter; };
struct KmtQuery { KmtHandle adapter; UINT type; void* data; UINT size; };
struct KmtAdapterPerf {                   // D3DKMT_ADAPTER_PERFDATA
    UINT32 physicalAdapterIndex;
    ULONGLONG memoryFrequency, maxMemoryFrequency, maxMemoryFrequencyOC, memoryBandwidth, pcieBandwidth;
    ULONG fanRpm, power, temperature;     // temperature in tenths of a degree C
    UCHAR powerStateOverride;
};
const UINT kQueryAdapterPerf = 62;        // KMTQAITYPE_ADAPTERPERFDATA
typedef LONG(APIENTRY* OpenFn)(KmtOpenFromLuid*);
typedef LONG(APIENTRY* QueryFn)(const KmtQuery*);
typedef LONG(APIENTRY* CloseFn)(const KmtClose*);

struct GpuTemp {
    QueryFn query = nullptr;
    CloseFn close = nullptr;
    KmtHandle adapter = 0;
    bool ok = false;
    void Open(LUID luid) {
        HMODULE gdi = GetModuleHandleW(L"gdi32.dll");
        if (!gdi) gdi = LoadLibraryW(L"gdi32.dll");
        auto open = (OpenFn)GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid");
        query = (QueryFn)GetProcAddress(gdi, "D3DKMTQueryAdapterInfo");
        close = (CloseFn)GetProcAddress(gdi, "D3DKMTCloseAdapter");
        KmtOpenFromLuid o = { luid, 0 };
        ok = open && query && close && open(&o) == 0;
        adapter = o.adapter;
    }
    double Read() {
        if (!ok) return -1;
        KmtAdapterPerf perf = {};
        KmtQuery q = { adapter, kQueryAdapterPerf, &perf, sizeof(perf) };
        if (query(&q) != 0 || perf.temperature == 0 || perf.temperature > 1500) { ok = false; return -1; }   // not reported: stop asking
        return perf.temperature / 10.0;
    }
    ~GpuTemp() { if (ok && close) { KmtClose c = { adapter }; close(&c); } }
};

std::wstring Lower(std::wstring s) { for (auto& c : s) c = (wchar_t)towlower(c); return s; }

// every instance of a wildcard counter, e.g. "\GPU Engine(*)\Utilization Percentage"
std::vector<std::pair<std::wstring, double>> Instances(PDH_HCOUNTER c) {
    std::vector<std::pair<std::wstring, double>> out;
    DWORD size = 0, count = 0;
    if (PdhGetFormattedCounterArrayW(c, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, nullptr) != PDH_MORE_DATA || !size) return out;
    std::vector<BYTE> buf(size);
    auto* items = (PDH_FMT_COUNTERVALUE_ITEM_W*)buf.data();
    if (PdhGetFormattedCounterArrayW(c, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &size, &count, items) != ERROR_SUCCESS) return out;
    for (DWORD i = 0; i < count; i++)
        if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || items[i].FmtValue.CStatus == PDH_CSTATUS_NEW_DATA)
            out.push_back({ Lower(items[i].szName), items[i].FmtValue.doubleValue });
    return out;
}

}  // namespace

void Sensors::Start(uint32_t luidLow, int32_t luidHigh, uint64_t vramBytes) {
    if (thread_.joinable()) return;
    luidLow_ = luidLow; luidHigh_ = luidHigh; vramBytes_ = vramBytes;
    stop_ = false;
    thread_ = std::thread([this] { Run(); });
}

void Sensors::Stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
}

Readings Sensors::Get() const {
    std::lock_guard<std::mutex> l(mu_);
    return r_;
}

void Sensors::Run() {
    wchar_t tag[64];   // how the GPU counters name our adapter
    swprintf(tag, 64, L"luid_0x%08x_0x%08x", (unsigned)luidHigh_, (unsigned)luidLow_);
    const std::wstring luid = tag;
    LUID l = { luidLow_, luidHigh_ };
    GpuTemp temp;
    if (luidLow_ || luidHigh_) temp.Open(l);

    PDH_HQUERY q = nullptr;
    PDH_HCOUNTER eng = nullptr, mem = nullptr, cpu = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &q) == ERROR_SUCCESS) {
        PdhAddEnglishCounterW(q, L"\\GPU Engine(*)\\Utilization Percentage", 0, &eng);
        PdhAddEnglishCounterW(q, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &mem);
        PdhAddEnglishCounterW(q, L"\\Processor Information(_Total)\\% Processor Utility", 0, &cpu);
        PdhCollectQueryData(q);   // rates need two samples
    }
    bool wasActive = false;
    while (!stop_) {
        for (int i = 0; i < 10 && !stop_; i++) Sleep(100);   // once a second, but quick to stop
        if (stop_) break;
        if (!active_) { wasActive = false; continue; }
        Readings r;
        if (q && PdhCollectQueryData(q) == ERROR_SUCCESS && wasActive) {
            // GPU usage like Task Manager: add up each engine type on our GPU, take the busiest type
            std::map<std::wstring, double> byType;
            for (auto& [name, v] : Instances(eng)) {
                if (name.find(luid) == std::wstring::npos) continue;
                size_t t = name.find(L"engtype_");
                byType[t == std::wstring::npos ? L"" : name.substr(t + 8)] += v;
            }
            if (!byType.empty()) {
                double best = 0;
                for (auto& [t, v] : byType) best = std::max(best, v);
                r.gpuPct = std::min(100.0, best);
            }
            double used = 0; bool any = false;
            for (auto& [name, v] : Instances(mem)) if (name.find(luid) != std::wstring::npos) { used += v; any = true; }
            if (any) { r.vramUsedGB = used / 1073741824.0; r.vramTotalGB = vramBytes_ / 1073741824.0; }
            PDH_FMT_COUNTERVALUE v;
            if (cpu && PdhGetFormattedCounterValue(cpu, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &v) == ERROR_SUCCESS)
                r.cpuPct = std::clamp(v.doubleValue, 0.0, 100.0);
        }
        wasActive = true;
        r.gpuTempC = temp.Read();
        MEMORYSTATUSEX ms = { sizeof(ms) };
        if (GlobalMemoryStatusEx(&ms)) {
            ULONGLONG installedKB = 0;   // the RAM you bought (Windows keeps a little of it back)
            r.ramTotalGB = GetPhysicallyInstalledSystemMemory(&installedKB) && installedKB ? installedKB / 1048576.0 : ms.ullTotalPhys / 1073741824.0;
            r.ramUsedGB = (ms.ullTotalPhys - ms.ullAvailPhys) / 1073741824.0;
        }
        std::lock_guard<std::mutex> lk(mu_);
        r_ = r;
    }
    if (q) PdhCloseQuery(q);
}
