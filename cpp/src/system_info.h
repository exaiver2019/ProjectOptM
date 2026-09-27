// Hardware and Windows detection (ported from the PowerShell version).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class CpuLayout { Unknown, Single, SingleX3D, DualCCD, DualX3D, Hybrid };

struct GpuInfo {
    std::string name;
    std::string vendor;      // "AMD" / "NVIDIA" / "Intel" / "Other"
    uint64_t    vramBytes = 0;
};

struct SystemInfo {
    // CPU
    std::string cpuName, cpuVendor;
    int cores = 0, threads = 0;
    CpuLayout layout = CpuLayout::Unknown;
    uint64_t allMask = 0, bestMask = 0, otherMask = 0;   // affinity masks (processor group 0)
    unsigned maxL3MB = 0, minL3MB = 0;
    bool canPin = false;
    // GPU
    std::vector<GpuInfo> gpus;                            // sorted, dedicated first
    // RAM
    unsigned ramGB = 0, ramMTs = 0, sticks = 0;
    std::string ramType, ramLayout;
    // Display
    int dispW = 0, dispH = 0, dispHz = 0, dispMaxHz = 0;
    // Windows
    std::string osName;
    unsigned osBuild = 0;
    // Intel / laptop extras
    uint32_t microcode = 0;      // CPU microcode revision (0 = unknown)
    bool hasBattery = false;     // laptop (or a PC on a UPS that reports as a battery)
    bool apo = false;            // Intel Application Optimization installed
    bool IsRaptorLake() const;   // 13th/14th gen Core i5/i7/i9 desktop
    bool IsArrowLake() const;    // Core Ultra 200S desktop

    void Detect();
    std::string LayoutText() const;       // "Dual-CCD, 3D V-Cache (96MB + 32MB L3)"
    std::string BestLabel() const;        // "V-Cache cores" / "P-cores" / "All cores"
    std::string OtherLabel() const;
    static std::string MaskText(uint64_t mask);   // "0-15, 32"
};
