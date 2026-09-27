// The Tweaks page: every tweak, the built-in presets, and which tweaks are on.
// Tweaks are applied when a game starts and undone when it closes ("changes apply to the next game").
#pragma once
#include <set>
#include <string>
#include <vector>
#include "data.h"
#include "system_info.h"

namespace tweakset {

struct Tweak {
    const char* id;
    const char* category;      // CPU / Memory / System / GPU / Power
    const char* name;
    const char* desc;
    const char* badge;         // warning chip ("" = none)
    int tier;                  // first built-in preset that turns it on: 0 Safe, 1 Balanced, 2 Aggressive, 3 never (opt-in)
};

// The long form shown when a tweak is expanded: what it does, what you gain, what it costs
struct Details {
    const char* what;
    const char* pros;
    const char* cons;
};

const std::vector<Tweak>& All();
const Details& DetailsOf(const std::string& id);
const std::vector<const char*>& Categories();
const Tweak* Find(const std::string& id);

// Built-in presets (locked). User presets live in AppData::tweakPresets.
const std::vector<std::string>& BuiltIns();              // "Safe", "Balanced", "Aggressive"
bool IsBuiltIn(const std::string& preset);
std::set<std::string> PresetTweaks(const AppData& d, const std::string& preset);   // as chosen, before availability

// Why a tweak can't be used on this PC ("" = it can)
std::string Unavailable(const std::string& id, const SystemInfo& sys, const AppData& d);

// Per-game tweaks: a game can follow the Tweaks page preset, pick another preset, or have its own set
bool PresetExists(const AppData& d, const std::string& preset);
std::string PresetFor(const AppData& d, const GameProfile* p);            // "Balanced", "Custom", ... (nullptr = Tweaks page)
std::set<std::string> ChosenFor(const AppData& d, const GameProfile* p);  // as chosen, before availability

// The tweaks that will actually run: the preset (the game's own if it has one), minus the ones this PC can't use
std::set<std::string> Active(const AppData& d, const SystemInfo& sys, const GameProfile* p = nullptr);

// Presets on disk (Export / Import): {"ProjectOptMTweaks": 1, "Name": "...", "On": [ids]}
std::string ExportJson(const std::string& name, const std::set<std::string>& on);
bool ImportJson(const std::string& text, std::string& name, std::set<std::string>& on);

}  // namespace tweakset
