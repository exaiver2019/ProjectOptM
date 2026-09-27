#include "checks.h"
#include "tweaks.h"
#include "util.h"
#include <cstdio>
#include <ctime>
#include <regex>
#include <windows.h>

namespace {

Check Make(Check::State s, std::string text, std::string tip, std::function<std::string()> fix = nullptr, std::string label = "Click to fix.") {
    Check c;
    c.state = s; c.text = std::move(text); c.tip = std::move(tip); c.fix = std::move(fix); c.fixLabel = std::move(label);
    return c;
}

std::function<std::string()> Link(const wchar_t* target) {
    std::wstring t = target;
    return [t] { util::OpenAsUser(t); return std::string(); };
}

// Xbox Game Bar installed for this user? -1 = couldn't tell
int GameBarInstalled() {
    HKEY k;
    const wchar_t* path = L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &k) != ERROR_SUCCESS) return -1;
    int found = 0;
    wchar_t name[512];
    for (DWORD i = 0;; i++) {
        DWORD len = 512;
        if (RegEnumKeyExW(k, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        if (_wcsnicmp(name, L"Microsoft.XboxGamingOverlay_", 28) == 0) { found = 1; break; }
    }
    RegCloseKey(k);
    return found;
}

// Driver date of a display adapter, from the display class key ("M-D-YYYY")
bool GpuDriverDate(const std::string& gpuName, int& y, int& m, int& d) {
    HKEY cls;
    const wchar_t* path = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &cls) != ERROR_SUCCESS) return false;
    bool ok = false;
    wchar_t sub[64];
    for (DWORD i = 0; !ok; i++) {
        DWORD len = 64;
        if (RegEnumKeyExW(cls, i, sub, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        if (wcslen(sub) != 4) continue;
        std::string desc = util::RegString(cls, sub, L"DriverDesc");
        if (desc.empty() || util::Lower(desc) != util::Lower(gpuName)) continue;
        std::string date = util::RegString(cls, sub, L"DriverDate");
        ok = sscanf(date.c_str(), "%d-%d-%d", &m, &d, &y) == 3;
    }
    RegCloseKey(cls);
    return ok;
}

}  // namespace

std::vector<Check> IntelChecks(const SystemInfo& sys) {
    std::vector<Check> list;
    if (sys.cpuVendor != "Intel") return list;
    char hex[16];
    snprintf(hex, sizeof(hex), "0x%X", sys.microcode);
    std::string mc = hex;

    // 13th/14th gen desktop: the Vmin-shift instability fixes (0x12B minimum, 0x12F latest)
    if (sys.IsRaptorLake() && sys.microcode) {
        if (sys.microcode < 0x12B)
            list.push_back(Make(Check::Warn, "Intel microcode " + mc + " - update your BIOS now",
                                "13th and 14th gen Core i5/i7/i9 desktop chips need microcode 0x12B or newer to prevent the instability and "
                                "degradation problem Intel confirmed in 2024. Update your motherboard BIOS - this can't be fixed from Windows."));
        else if (sys.microcode < 0x12F)
            list.push_back(Make(Check::Warn, "Intel microcode " + mc + " - newer fix available",
                                "You have Intel's main fix for the 13th/14th gen instability problem. Microcode 0x12F adds a further fix for "
                                "light, long-running workloads. Get it with the latest BIOS for your motherboard."));
        else
            list.push_back(Make(Check::Ok, "Intel microcode " + mc, "Includes Intel's fixes for the 13th/14th gen instability problem."));
    }

    // Core Ultra 200S desktop: Intel's performance fixes need microcode 0x114+ and Windows 11 24H2
    if (sys.IsArrowLake()) {
        bool mcOk = sys.microcode >= 0x114, winOk = sys.osBuild >= 26100;
        if (sys.microcode && mcOk && winOk)
            list.push_back(Make(Check::Ok, "Core Ultra 200S fixes installed", "Microcode " + mc + " and Windows 11 24H2 include Intel's gaming performance fixes for these chips."));
        else if (sys.microcode || !winOk) {
            std::string need;
            if (sys.microcode && !mcOk) need += "a BIOS update with microcode 0x114 or newer (you have " + mc + ")";
            if (!winOk) need += std::string(need.empty() ? "" : " and ") + "Windows 11 24H2 or newer";
            list.push_back(Make(Check::Warn, "Core Ultra 200S performance fix missing",
                                "Intel found that early firmware and Windows versions cost these chips a lot of gaming performance. You need " + need +
                                ". Keep Windows Update current too - part of the fix ships there.",
                                !winOk ? Link(L"ms-settings:windowsupdate") : nullptr, "Click to open Windows Update."));
        }
    }

    // Intel Application Optimization: tunes supported games itself
    if (sys.apo)
        list.push_back(Make(Check::Info, "Intel APO installed",
                            "Intel Application Optimization tunes thread placement for the games it supports. So the two don't fight, "
                            "Project OptM uses soft core pinning (CPU sets) instead of locking games to the P-cores."));
    return list;
}

std::vector<Check> RunChecks(const SystemInfo& sys, const IniSettings& settings) {
    std::vector<Check> list;
    char b[256];

    // RAM speed
    if (sys.ramMTs) {
        unsigned okSpeed = sys.ramType == "DDR5" ? 5600 : 3000;
        snprintf(b, sizeof(b), "%u MT/s", sys.ramMTs);
        if (sys.ramMTs >= okSpeed) list.push_back(Make(Check::Ok, "RAM profile on (" + std::string(b) + ")", "EXPO/XMP is active."));
        else list.push_back(Make(Check::Warn, "RAM at " + std::string(b) + " - EXPO/XMP may be off",
                                 "Enable EXPO (AMD) or XMP (Intel) in your BIOS to run your RAM at its rated speed. This one has to be done in the BIOS."));
    }
    std::string gb = std::to_string(sys.ramGB) + " GB RAM";
    if (sys.ramGB >= 48)      list.push_back(Make(Check::Info, gb + " - cleanup not needed", "With this much RAM, standby cleanup is skipped for all games."));
    else if (sys.ramGB <= 16) list.push_back(Make(Check::Info, gb + " - extra cleanup on", "Standby RAM is cleared twice as often in heavy games."));
    else                      list.push_back(Make(Check::Info, gb + " - cleanup for heavy games", "Standby RAM is cleared on a timer in games that need it."));

    // Refresh rate (read fresh - it can change while we run)
    DEVMODEW cur = {}; cur.dmSize = sizeof(cur);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &cur) && cur.dmDisplayFrequency > 1) {
        int hz = cur.dmDisplayFrequency, maxHz = hz;
        DEVMODEW dm = {}; dm.dmSize = sizeof(dm);
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); i++)
            if (dm.dmPelsWidth == cur.dmPelsWidth && dm.dmPelsHeight == cur.dmPelsHeight && !(dm.dmDisplayFlags & DM_INTERLACED))
                maxHz = std::max(maxHz, (int)dm.dmDisplayFrequency);
        if (maxHz > hz) {
            snprintf(b, sizeof(b), "Your monitor can do %d Hz but Windows is running it at %d Hz. Pick the highest rate under 'Choose a refresh rate'.", maxHz, hz);
            list.push_back(Make(Check::Warn, "Display at " + std::to_string(hz) + " Hz (supports " + std::to_string(maxHz) + " Hz)", b,
                                Link(L"ms-settings:display-advanced"), "Click to open display settings."));
        } else {
            list.push_back(Make(Check::Ok, "Display at max refresh (" + std::to_string(hz) + " Hz)", "Your main display is running at its highest refresh rate."));
        }
    }

    // Power plan
    std::string plan = tweaks::ActivePlan();
    std::string planName = plan.empty() ? "Unknown" : tweaks::PlanName(plan);
    if (sys.layout == CpuLayout::DualX3D) {
        if (plan == tweaks::kBalanced) list.push_back(Make(Check::Ok, "Power plan: Balanced", "Correct for X3D - lets the V-Cache core parking work."));
        else list.push_back(Make(Check::Warn, "Power plan: " + planName,
                                 "Dual-CCD X3D chips work best on Balanced. Other plans can stop the V-Cache core parking from working.",
                                 [] { tweaks::SetPlan(tweaks::kBalanced); return std::string(); }, "Click to switch to Balanced."));
        if (tweaks::ServiceExists("amd3dv") || tweaks::ServiceExists("v-cache"))
            list.push_back(Make(Check::Ok, "AMD V-Cache driver installed", "The AMD 3D V-Cache Performance Optimizer is present."));
        else list.push_back(Make(Check::Warn, "AMD V-Cache driver not found",
                                 "Install the latest AMD chipset driver - it includes the 3D V-Cache Performance Optimizer.",
                                 Link(L"https://www.amd.com/en/support/download/drivers.html"), "Click to open AMD drivers."));
        if (GameBarInstalled() == 0)
            list.push_back(Make(Check::Warn, "Xbox Game Bar missing",
                                "AMD's V-Cache driver uses Game Bar to tell when a game is running. Without it, games may land on the wrong cores.",
                                Link(L"ms-windows-store://pdp/?ProductId=9NZKPSTSNW4P"), "Click to open it in the Store."));
    } else {
        list.push_back(Make(Check::Info, "Power plan: " + planName,
                            settings.powerPlan == "off" ? "Not changed while gaming." : "Switches to High performance while a game runs, then back."));
    }

    // Game Mode
    DWORD v = 1;
    if (util::RegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\GameBar", L"AutoGameModeEnabled", v) && v == 0)
        list.push_back(Make(Check::Warn, "Game Mode off", "Game Mode stops Windows Update from installing drivers mid-game and gives games scheduling priority.",
                            [] {
                                util::SetRegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\GameBar", L"AutoGameModeEnabled", 1);
                                util::SetRegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\GameBar", L"AllowAutoGameMode", 1);
                                return std::string();
                            }, "Click to turn it on."));
    else list.push_back(Make(Check::Ok, "Game Mode on", "Windows Game Mode is enabled."));

    // Background recording
    v = 0;
    const wchar_t* dvr = L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR";
    if (util::RegDword(HKEY_CURRENT_USER, dvr, L"HistoricalCaptureEnabled", v) && v == 1)
        list.push_back(Make(Check::Warn, "Background recording on",
                            "Game Bar's 'Record what happened' constantly records gameplay, which costs FPS. You can still record clips manually with it off.",
                            [dvr] { util::SetRegDword(HKEY_CURRENT_USER, dvr, L"HistoricalCaptureEnabled", 0); return std::string(); }, "Click to turn it off."));
    else list.push_back(Make(Check::Ok, "Background recording off", "Game Bar isn't constantly recording in the background."));

    // Memory Integrity (security trade-off - info only)
    v = 0;
    if (util::RegDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity", L"Enabled", v) && v == 1)
        list.push_back(Make(Check::Info, "Memory Integrity on",
                            "Memory Integrity (Core isolation) can cost a few percent FPS in some games, but it's a real security feature. Turning it off is a trade-off - your call.",
                            Link(L"windowsdefender://coreisolation"), "Click to open Core isolation settings."));

    // Multiple GPUs
    if (sys.gpus.size() >= 2 && settings.forceGpu)
        list.push_back(Make(Check::Info, "Games locked to " + sys.gpus[0].name,
                            "You also have " + sys.gpus[1].name + " active. Each game is set to always use your main GPU (Settings > Display > Graphics)."));

    // ----- Maker-specific checks -----
    const std::string gpuVendor = sys.gpus.empty() ? "Other" : sys.gpus[0].vendor;
    int y, m, d;
    if (!sys.gpus.empty() && GpuDriverDate(sys.gpus[0].name, y, m, d)) {
        tm t = {}; t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d; t.tm_hour = 12;
        double days = difftime(time(nullptr), mktime(&t)) / 86400.0;
        static const char* M[] = { "January","February","March","April","May","June","July","August","September","October","November","December" };
        std::string when = (m >= 1 && m <= 12 ? std::string(M[m - 1]) : "?") + " " + std::to_string(y);
        const wchar_t* link = gpuVendor == "NVIDIA" ? L"https://www.nvidia.com/en-us/drivers/"
                            : gpuVendor == "AMD"    ? L"https://www.amd.com/en/support/download/drivers.html"
                            : gpuVendor == "Intel"  ? L"https://www.intel.com/content/www/us/en/support/detect.html" : nullptr;
        if (days > 180)
            list.push_back(Make(Check::Warn, gpuVendor + " driver is " + std::to_string((int)(days / 30)) + " months old",
                                "Your graphics driver is from " + when + ". New drivers often include fixes and performance improvements for recent games.",
                                link ? Link(link) : nullptr, "Click to open the driver download page."));
        else
            list.push_back(Make(Check::Ok, gpuVendor + " driver recent (" + when.substr(0, 3) + " " + std::to_string(y) + ")", "Your graphics driver is less than 6 months old."));
    }
    // NVIDIA RTX 40/50: DLSS Frame Generation needs hardware-accelerated GPU scheduling
    if (gpuVendor == "NVIDIA" && std::regex_search(sys.gpus[0].name, std::regex("RTX\\s*(40|50)\\d\\d"))) {
        const wchar_t* gd = L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers";
        v = 0;
        if (util::RegDword(HKEY_LOCAL_MACHINE, gd, L"HwSchMode", v) && v == 1)
            list.push_back(Make(Check::Warn, "GPU scheduling off",
                                "NVIDIA DLSS Frame Generation needs Hardware-accelerated GPU scheduling. Turning it on takes effect after a restart.",
                                [gd] { util::SetRegDword(HKEY_LOCAL_MACHINE, gd, L"HwSchMode", 2); return std::string("  Restart your PC to finish turning on GPU scheduling"); },
                                "Click to turn it on (restart needed)."));
        else list.push_back(Make(Check::Ok, "GPU scheduling on", "Needed for DLSS Frame Generation on RTX 40 and 50 series cards."));
    }
    // Intel: microcode fixes, APO
    for (auto& c : IntelChecks(sys)) list.push_back(c);

    // Laptops: gaming on battery
    SYSTEM_POWER_STATUS ps;
    if (sys.hasBattery && GetSystemPowerStatus(&ps) && ps.ACLineStatus == 0)
        list.push_back(Make(Check::Warn, "Running on battery",
                            "Laptops cut CPU and GPU power a lot on battery, so games run much slower. Plug in the charger for gaming."));
    // Intel Arc: Resizable BAR
    if (gpuVendor == "Intel" && sys.gpus[0].name.find("Arc") != std::string::npos)
        list.push_back(Make(Check::Info, "Intel Arc: keep Resizable BAR on",
                            "Arc cards lose a lot of performance without Resizable BAR. Make sure it and Above 4G Decoding are turned on in your BIOS."));
    return list;
}
