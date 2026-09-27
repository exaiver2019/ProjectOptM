// Graphics driver facts, read-only: its version, the shader cache folders and their size, and the
// global AMD Adrenalin settings that matter for games (Radeon Chill, Anti-Lag, Boost, VSync).
// Nothing here changes a driver setting.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "system_info.h"

namespace driverinfo {

std::string Version(const std::string& gpuName);            // "32.0.31035.1003" ("" = unknown)

std::vector<std::wstring> ShaderCacheDirs(const SystemInfo& sys);
struct CacheSize { uint64_t bytes = 0; int files = 0; };
CacheSize ShaderCacheSize(const std::vector<std::wstring>& dirs);   // walks the folders - run off the UI thread

struct Setting {
    std::string name, value;   // "Radeon Chill", "On"
    std::string tip;           // what it means for games ("" = nothing to say)
    bool flag = false;         // worth a look
};
// Global Adrenalin settings of the first AMD card (empty for other makers or if they can't be read)
std::vector<Setting> AmdSettings(const SystemInfo& sys);

}  // namespace driverinfo
