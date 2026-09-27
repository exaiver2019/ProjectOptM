// System health checks shown on the System page (re-run when the window regains focus).
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "data.h"
#include "system_info.h"

struct Check {
    enum State { Ok, Warn, Info } state = Info;
    std::string text, tip;
    std::function<std::string()> fix;   // returns a line for the activity log (may be empty)
    std::string fixLabel = "Click to fix.";
};

std::vector<Check> RunChecks(const SystemInfo& sys, const IniSettings& settings);

// Intel-specific checks, from detected values only (no system calls - testable with any CPU name)
std::vector<Check> IntelChecks(const SystemInfo& sys);
