#include "system_info.h"
#include "util.h"

#include <algorithm>
#include <cstdio>
#include <dxgi.h>
#include <regex>
#include <windows.h>

namespace {

// ---------- CPU topology (cache layout + core efficiency classes) ----------
void DetectTopology(SystemInfo& si) {
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &len);
    if (!len) return;
    std::vector<uint8_t> buf(len);
    if (!GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) return;

    std::vector<uint64_t> l3Masks, coreMasks;
    std::vector<uint32_t> l3Sizes;
    std::vector<uint8_t>  coreEff;
    uint64_t all = 0;
    uint8_t maxEff = 0;
    int cores = 0;

    // Raw offsets are used on purpose so this works with any SDK's struct layout.
    for (DWORD off = 0; off < len;) {
        const uint8_t* p = buf.data() + off;
        DWORD rel  = *(const DWORD*)(p + 0);
        DWORD size = *(const DWORD*)(p + 4);
        if (!size) break;
        if (rel == 0) {                                   // RelationProcessorCore
            uint8_t  eff = p[9];
            uint64_t m   = *(const uint64_t*)(p + 32);
            uint16_t grp = *(const uint16_t*)(p + 40);
            if (grp == 0) { all |= m; coreMasks.push_back(m); coreEff.push_back(eff); maxEff = std::max(maxEff, eff); }
            cores++;
        } else if (rel == 2) {                            // RelationCache
            uint8_t  level = p[8];
            uint32_t csize = *(const uint32_t*)(p + 12);
            int32_t  type  = *(const int32_t*)(p + 16);
            uint64_t m     = *(const uint64_t*)(p + 40);
            uint16_t grp   = *(const uint16_t*)(p + 48);
            if (level == 3 && type == 0 && grp == 0) { l3Masks.push_back(m); l3Sizes.push_back(csize); }
        }
        off += size;
    }

    si.cores = cores;
    si.allMask = all;
    si.bestMask = all;
    si.otherMask = 0;
    if (l3Sizes.empty()) { si.layout = CpuLayout::Single; }
    size_t big = 0;
    uint32_t maxS = 0, minS = UINT32_MAX;
    for (size_t i = 0; i < l3Sizes.size(); i++) {
        if (l3Sizes[i] > maxS) { maxS = l3Sizes[i]; big = i; }
        minS = std::min(minS, l3Sizes[i]);
    }
    si.maxL3MB = maxS / (1024 * 1024);
    si.minL3MB = l3Sizes.empty() ? 0 : minS / (1024 * 1024);

    if (maxEff > 0) {                                     // Intel hybrid: P-cores have the higher class
        uint64_t pm = 0;
        for (size_t i = 0; i < coreMasks.size(); i++) if (coreEff[i] == maxEff) pm |= coreMasks[i];
        si.bestMask = pm; si.otherMask = all & ~pm;
        si.layout = CpuLayout::Hybrid;
    } else if (l3Masks.size() >= 2 && maxS > minS && si.cpuVendor == "AMD") {   // AMD dual-CCD with V-Cache on one CCD
        si.bestMask = l3Masks[big] & all; si.otherMask = all & ~l3Masks[big];
        si.layout = CpuLayout::DualX3D;
    } else if (l3Masks.size() >= 2) {
        si.layout = CpuLayout::DualCCD;
    } else if (l3Masks.size() == 1 && maxS >= 64u * 1024 * 1024 && si.cpuVendor == "AMD") {
        si.layout = CpuLayout::SingleX3D;
    } else if (si.layout == CpuLayout::Unknown) {
        si.layout = CpuLayout::Single;
    }
    si.canPin = si.threads <= 64 && si.bestMask != si.allMask && si.bestMask != 0;
}

// ---------- RAM from the SMBIOS tables (type 17 = memory device) ----------
void DetectRam(SystemInfo& si) {
    MEMORYSTATUSEX ms = { sizeof(ms) };
    GlobalMemoryStatusEx(&ms);

    const DWORD kRSMB = ('R' << 24) | ('S' << 16) | ('M' << 8) | 'B';   // raw SMBIOS provider
    UINT size = GetSystemFirmwareTable(kRSMB, 0, nullptr, 0);
    std::vector<uint8_t> buf(size);
    uint64_t totalMB = 0;
    std::vector<unsigned> stickGB;
    unsigned speed = 0, memType = 0;
    if (size && GetSystemFirmwareTable(kRSMB, 0, buf.data(), size) == size && size > 8) {
        const uint8_t* p   = buf.data() + 8;             // skip RawSMBIOSData header
        const uint8_t* end = buf.data() + size;
        while (p + 4 <= end) {
            uint8_t type = p[0], len = p[1];
            if (len < 4 || p + len > end) break;
            if (type == 17 && len > 0x15) {
                uint16_t sz = *(const uint16_t*)(p + 0x0C);
                uint64_t mb = 0;
                if (sz == 0x7FFF && len >= 0x20) mb = *(const uint32_t*)(p + 0x1C);
                else if (sz != 0 && sz != 0xFFFF) mb = (sz & 0x8000) ? (sz & 0x7FFF) / 1024 : sz;
                if (mb) {
                    totalMB += mb;
                    stickGB.push_back((unsigned)((mb + 512) / 1024));
                    memType = p[0x12];
                    unsigned sp = *(const uint16_t*)(p + 0x15);
                    if (len >= 0x22) { unsigned cfg = *(const uint16_t*)(p + 0x20); if (cfg && cfg != 0xFFFF) sp = cfg; }
                    speed = std::max(speed, sp);
                }
            }
            if (type == 127) break;                          // end of table
            const uint8_t* s = p + len;                       // skip the string area (ends with two NULs)
            while (s + 1 < end && !(s[0] == 0 && s[1] == 0)) s++;
            p = s + 2;
        }
    }
    si.ramGB = totalMB ? (unsigned)((totalMB + 512) / 1024) : (unsigned)((ms.ullTotalPhys + (512ull << 20)) >> 30);
    si.ramMTs = speed;
    si.sticks = (unsigned)stickGB.size();
    si.ramType = memType == 0x22 ? "DDR5" : memType == 0x1A ? "DDR4" : memType == 0x18 ? "DDR3" : (speed >= 4800 ? "DDR5" : "DDR4");
    if (!stickGB.empty()) {
        bool same = std::all_of(stickGB.begin(), stickGB.end(), [&](unsigned g) { return g == stickGB[0]; });
        char b[64];
        if (same) snprintf(b, sizeof(b), "%zu x %u GB", stickGB.size(), stickGB[0]);
        else { std::string t; for (size_t i = 0; i < stickGB.size(); i++) t += (i ? " + " : "") + std::to_string(stickGB[i]); snprintf(b, sizeof(b), "%s GB", t.c_str()); }
        si.ramLayout = b;
    }
}

// ---------- GPUs through DXGI (reports real VRAM, unlike WMI) ----------
void DetectGpus(SystemInfo& si) {
    IDXGIFactory1* f = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&f))) return;
    IDXGIAdapter1* a = nullptr;
    for (UINT i = 0; f->EnumAdapters1(i, &a) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 d;
        if (SUCCEEDED(a->GetDesc1(&d)) && !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            GpuInfo g;
            g.name = util::Trim(util::Narrow(d.Description));
            g.vramBytes = d.DedicatedVideoMemory;
            g.luidLow = d.AdapterLuid.LowPart;
            g.luidHigh = d.AdapterLuid.HighPart;
            g.vendor = d.VendorId == 0x1002 ? "AMD" : d.VendorId == 0x10DE ? "NVIDIA" : d.VendorId == 0x8086 ? "Intel" : "Other";
            bool dup = std::any_of(si.gpus.begin(), si.gpus.end(), [&](const GpuInfo& x) { return x.name == g.name; });
            if (!dup) si.gpus.push_back(g);
        }
        a->Release();
    }
    f->Release();
    std::stable_sort(si.gpus.begin(), si.gpus.end(), [](const GpuInfo& x, const GpuInfo& y) { return x.vramBytes > y.vramBytes; });
}

// ---------- Main display: current and highest refresh at this resolution ----------
void DetectDisplay(SystemInfo& si) {
    DEVMODEW cur = {}; cur.dmSize = sizeof(cur);
    if (!EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &cur)) return;
    si.dispW = cur.dmPelsWidth; si.dispH = cur.dmPelsHeight; si.dispHz = cur.dmDisplayFrequency;
    si.dispMaxHz = si.dispHz;
    DEVMODEW dm = {}; dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); i++) {
        if (dm.dmPelsWidth == cur.dmPelsWidth && dm.dmPelsHeight == cur.dmPelsHeight && !(dm.dmDisplayFlags & DM_INTERLACED))
            si.dispMaxHz = std::max(si.dispMaxHz, (int)dm.dmDisplayFrequency);
    }
}

// ---------- Windows version (ProductName says "Windows 10" on 11, so use the build) ----------
void DetectOs(SystemInfo& si) {
    typedef LONG(WINAPI * RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    RTL_OSVERSIONINFOW v = { sizeof(v) };
    if (auto fn = (RtlGetVersionFn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion")) fn(&v);
    const wchar_t* key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    std::string edition = util::RegString(HKEY_LOCAL_MACHINE, key, L"EditionID");
    std::string ver = util::RegString(HKEY_LOCAL_MACHINE, key, L"DisplayVersion");
    std::string name = v.dwBuildNumber >= 22000 ? "Windows 11" : "Windows 10";
    if (edition == "Professional") name += " Pro";
    else if (edition == "Core") name += " Home";
    else if (!edition.empty()) name += " " + edition;
    if (!ver.empty()) name += " " + ver;
    si.osName = name + " (build " + std::to_string(v.dwBuildNumber) + ")";
    si.osBuild = v.dwBuildNumber;
}

// ---------- Microcode revision, battery, Intel APO ----------
void DetectExtras(SystemInfo& si) {
    BYTE rev[16] = {};
    DWORD size = sizeof(rev);
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"Update Revision",
                     RRF_RT_REG_BINARY, nullptr, rev, &size) == ERROR_SUCCESS && size >= 8) {
        si.microcode = *(uint32_t*)(rev + 4);
        if (si.microcode == 0) si.microcode = *(uint32_t*)rev;
    }
    SYSTEM_POWER_STATUS ps;
    si.hasBattery = GetSystemPowerStatus(&ps) && ps.BatteryFlag != 128 && ps.BatteryFlag != 255;
    si.apo = si.cpuVendor == "Intel" &&
             (util::AppxInstalled(L"AppUp.IntelApplicationOptimization") == 1 || util::AppxInstalled(L"AppUp.IntelAPO") == 1);
}

}  // namespace

void SystemInfo::Detect() {
    cpuName = util::RegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString");
    std::string vendorId = util::RegString(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"VendorIdentifier");
    cpuVendor = vendorId.find("AMD") != std::string::npos ? "AMD" : vendorId.find("Intel") != std::string::npos ? "Intel" : "Other";
    SYSTEM_INFO sys; GetSystemInfo(&sys);
    threads = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (!threads) threads = (int)sys.dwNumberOfProcessors;
    DetectTopology(*this);
    DetectRam(*this);
    DetectGpus(*this);
    DetectDisplay(*this);
    DetectOs(*this);
    DetectExtras(*this);
}

bool SystemInfo::IsRaptorLake() const {
    return cpuVendor == "Intel" && std::regex_search(cpuName, std::regex("\\bi[579]-1[34]\\d{3}(KS|KF|K|F|T)?\\b"));
}

bool SystemInfo::IsArrowLake() const {
    // desktop Core Ultra 200S (e.g. "Core(TM) Ultra 9 285K"); laptop chips end in H/HX/U/V
    return cpuVendor == "Intel" && std::regex_search(cpuName, std::regex("Ultra [579] 2\\d\\d(K|KF|F|T)?(\\s|$)"));
}

std::string SystemInfo::LayoutText() const {
    char b[96];
    switch (layout) {
        case CpuLayout::DualX3D:   snprintf(b, sizeof(b), "Dual-CCD, 3D V-Cache (%uMB + %uMB L3)", maxL3MB, minL3MB); return b;
        case CpuLayout::Hybrid:    return "Hybrid (P-cores + E-cores)";
        case CpuLayout::DualCCD:   snprintf(b, sizeof(b), "Dual-CCD (%uMB L3 each)", maxL3MB); return b;
        case CpuLayout::SingleX3D: snprintf(b, sizeof(b), "3D V-Cache (%uMB L3)", maxL3MB); return b;
        default:                   return "Single cluster";
    }
}

std::string SystemInfo::BestLabel() const {
    if (!canPin) return "All cores";
    return layout == CpuLayout::DualX3D ? "V-Cache cores" : layout == CpuLayout::Hybrid ? "P-cores" : "All cores";
}

std::string SystemInfo::OtherLabel() const {
    if (!canPin) return "All cores";
    return layout == CpuLayout::DualX3D ? "frequency cores" : layout == CpuLayout::Hybrid ? "E-cores" : "All cores";
}

std::string SystemInfo::MaskText(uint64_t mask) {
    std::string out;
    int start = -1;
    for (int i = 0; i <= 64; i++) {
        bool on = i < 64 && ((mask >> i) & 1);
        if (on && start < 0) start = i;
        if (!on && start >= 0) {
            if (!out.empty()) out += ", ";
            out += start == i - 1 ? std::to_string(start) : std::to_string(start) + "-" + std::to_string(i - 1);
            start = -1;
        }
    }
    return out.empty() ? "none" : out;
}
