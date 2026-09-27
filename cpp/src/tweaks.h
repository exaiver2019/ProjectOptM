// System-wide switches: services, power plans, launch priority (IFEO), GPU preference.
#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace tweaks {

// Services
bool ServiceRunning(const std::string& name);
bool StopService(const std::string& name);         // doesn't wait for it to finish stopping
bool StartService(const std::string& name);
bool ServiceExists(const std::string& nameOrDisplayPattern);   // name prefix or display-name substring, any case

// Power plans (GUIDs in powercfg's lower-case text form)
extern const char* kBalanced;
extern const char* kHighPerformance;
extern const char* kUltimate;
std::string ActivePlan();
std::string PlanName(const std::string& guid);
bool PlanExists(const std::string& guid);
bool SetPlan(const std::string& guid);

// Launch priority: Image File Execution Options\<exe>\PerfOptions\CpuPriorityClass
bool SetLaunchPriority(const std::string& exe, const std::string& priority);   // exe includes ".exe"
bool HasLaunchPriority(const std::string& exe, const std::string& priority);
void RemoveLaunchPriority(const std::string& exe);

// Windows' per-app graphics preference (Settings > Display > Graphics)
bool PreferDedicatedGpu(const std::wstring& exePath);   // true if it was changed
std::wstring GpuPreference(const std::wstring& exePath); // current value ("" = none)
void SetGpuPreference(const std::wstring& exePath, const std::wstring& value);   // "" removes it

// ---- Session tweaks. Each Set* returns a backup string ("" if nothing changed);
// Restore(backup) puts it back. Backups are saved so a crash can be undone on the next start.
std::string SetRegDword(HKEY root, const std::wstring& key, const std::wstring& value, DWORD data);
std::string SetRegString(HKEY root, const std::wstring& key, const std::wstring& value, const std::wstring& data);
std::string SetPowerValue(const GUID& sub, const GUID& setting, DWORD value);   // active plan, AC
std::string SetVisualEffects(bool on);
std::string AddDefenderExclusion(const std::wstring& folder);
std::string SetPowerMode(bool bestPerformance);          // Settings > System > Power > Power mode
std::string SetTransparency(bool on);                    // Settings > Personalization > Colors > Transparency effects
std::string SetMouseAcceleration(bool on);               // "Enhance pointer precision"
std::string SetAccessibilityHotkeys(bool on);            // Shift x5 (Sticky Keys), hold Shift (Filter Keys), hold Num Lock (Toggle Keys)
void Restore(const std::string& backup);

bool SetTimerResolution(bool fine);                     // 0.5 ms while fine = true
bool EnableGlobalTimerRequests();                       // Windows 11: lets it apply system-wide; true if newly set (reboot)

// Fullscreen optimizations (AppCompatFlags layer on the exe - takes effect next launch)
bool SetFullscreenOptimizationsOff(const std::wstring& exePath, bool off);   // true if changed

extern const GUID kSubProcessor, kCoreParkingMin, kIdleDisable, kBoostMode, kEnergyPref;
extern const GUID kSubPciExpress, kLinkStatePower;

}  // namespace tweaks
