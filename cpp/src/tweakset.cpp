#include "tweakset.h"
#include "json.h"
#include <map>

namespace tweakset {

// Safe = what Project OptM has always done. Balanced adds more session tweaks.
// Aggressive adds CPU power-plan tuning (not on X3D chips). Tier 3 is never in a preset.
const std::vector<Tweak>& All() {
    static const std::vector<Tweak> list = {
        // ---- CPU
        { "pinning",     "CPU", "Core pinning",          "Pins the game to the best cores for your CPU", "", 0 },
        { "priority",    "CPU", "Priority boost",        "The game runs at its profile's priority (Above Normal by default)", "", 0 },
        { "bg_priority", "CPU", "Background apps low",   "Browsers, Discord, launchers and your background list run at low priority", "", 0 },
        { "bg_affinity", "CPU", "Background affinity",   "Keeps background apps off the game's cores", "", 0 },
        { "soft_pin",    "CPU", "Soft core pinning",     "The game prefers the best cores instead of being locked to them", "", 3 },
        { "timer",       "CPU", "Timer resolution",      "0.5 ms system timer for steadier frame pacing", "", 1 },
        { "io_priority", "CPU", "I/O priority",          "High disk priority for the game", "", 1 },
        { "mmcss",       "CPU", "MMCSS Games profile",   "Games that use the multimedia scheduler get top GPU and CPU priority", "", 1 },
        { "explorer",    "CPU", "Explorer priority",     "Lowers explorer.exe while you play", "", 1 },
        { "smt",         "CPU", "SMT scheduling",        "Prefers one thread per physical core for the game", "", 2 },
        { "unpark",      "CPU", "Core unparking",        "Keeps every core awake while you play", "More power", 2 },
        { "cstate",      "CPU", "C-state disable",       "Blocks deep CPU idle states mid-frame", "More heat", 2 },
        { "maxboost",    "CPU", "Max boost",             "Aggressive boost policy, energy preference 0", "More power", 2 },
        // ---- Memory
        { "standby",     "Memory", "Standby cleaner",    "One standby-list flush at game launch", "", 0 },
        { "ram_cleaner", "Memory", "RAM cleaner",        "Periodic cleaning while playing (games with ramcleanup set)", "Can cause stutter", 0 },
        { "prefetch",    "Memory", "Prefetch off",       "Pauses SysMain prefetching during play", "", 0 },
        { "ws_trim",     "Memory", "Working-set trim",   "Trims background apps' memory once the game has loaded", "Can cause stutter", 1 },
        // ---- System
        { "services",    "System", "Service suspend",    "Pauses Windows Search, telemetry and your pause_services list", "", 0 },
        { "updates",     "System", "Windows Update pause", "Stops update downloads while you play", "", 0 },
        { "close_apps",  "System", "Close apps",         "Closes the apps on your close list when a game starts", "", 0 },
        { "dvr",         "System", "Game DVR capture off", "Turns off background capture hooks (Game Bar itself stays)", "", 1 },
        { "visual",      "System", "Visual effects off", "Turns off window animations while you play", "", 1 },
        { "fso",         "System", "Fullscreen optimizations off", "Per-game exclusive fullscreen behavior", "Next launch", 1 },
        { "audio",       "System", "Low-latency audio",  "Runs the Windows audio engine at High priority", "", 1 },
        { "defender",    "System", "Defender exclusion", "Skips scanning the game folder while playing", "Security trade-off", 3 },
        // ---- GPU
        { "gpu_lock",    "GPU", "Dedicated GPU lock",    "Games always use your graphics card, not integrated graphics", "", 0 },
        { "vendor_apps", "GPU", "GPU maker apps low",    "Radeon Software, NVIDIA app or Intel Graphics Software run at low priority", "", 0 },
        { "dwm",         "GPU", "DWM priority",          "Runs the desktop compositor at High", "", 1 },
        // ---- Power
        { "power_plan",  "Power", "High performance plan", "Switches to High performance while you play", "", 0 },
        { "ecoqos",      "Power", "Power throttling off", "Stops EcoQoS from slowing the game down", "", 0 },
        { "bg_eco",      "Power", "Background efficiency mode", "Background apps run in Windows efficiency mode while you play", "", 1 },
        { "power_mode",  "Power", "Best performance power mode", "Laptops: Windows power mode set to Best performance while you play", "", 1 },
    };
    return list;
}

const Details& DetailsOf(const std::string& id) {
    static const std::map<std::string, Details> d = {
        { "pinning", {
            "Restricts the game to the cores that run games best: the 3D V-Cache CCD on dual-CCD X3D chips, or the P-cores on Intel 12th gen and newer.",
            "Keeps the game on the cores with the big cache or the fastest clocks, and stops Windows moving its threads to slower cores or the other CCD. Usually raises 1% lows.",
            "A game that really uses more than 16 threads loses the other cores. Not used for anti-cheat games." } },
        { "priority", {
            "Raises the game's CPU priority class to what its profile says (Above Normal unless you changed it).",
            "The game wins when it competes with other apps for CPU time, so background activity causes fewer hitches.",
            "Small gain on a quiet PC. High priority can make the mouse and other apps feel sluggish when the game maxes out the CPU. Not used for anti-cheat games." } },
        { "bg_priority", {
            "Sets browsers, Discord, Spotify, launchers, your background list and your GPU maker's helper apps to Below Normal priority while you play.",
            "Background work yields to the game instead of taking CPU time from it.",
            "Those apps can react more slowly while you play (for example a video in a browser on your second screen)." } },
        { "bg_affinity", {
            "Moves background apps onto the cores the game isn't using: the frequency CCD on dual-CCD X3D chips, or the E-cores on Intel hybrid chips.",
            "The game's cores are left free for the game, which helps frame pacing.",
            "Background apps get fewer cores - streaming or recording software on the same PC may struggle." } },
        { "timer", {
            "Asks Windows for a 0.5 ms system timer instead of the default 15.6 ms while you play. On Windows 11 this sets GlobalTimerResolutionRequests once, which needs one restart.",
            "Sleeps and waits in games and frame limiters wake up more precisely, which can steady frame pacing and lower input latency.",
            "Slightly higher CPU power use while playing. Many modern games already raise the timer themselves, so the gain is often small." } },
        { "io_priority", {
            "Gives the game High disk (I/O) priority.",
            "Asset streaming and loading win against background disk activity like indexing and updates, which can reduce streaming stutter.",
            "Other apps' disk access can slow down while the game loads a lot. Only has an effect on slower drives. Not used for anti-cheat games." } },
        { "mmcss", {
            "Changes Windows' multimedia scheduler 'Games' task to GPU Priority 8, Priority 6 and High scheduling category while you play, then puts the original values back.",
            "Games and engines that register their threads with MMCSS get higher CPU and GPU scheduling priority.",
            "Only affects games that use MMCSS - many don't, so often nothing changes. Needs admin." } },
        { "explorer", {
            "Lowers explorer.exe (the taskbar, desktop and file windows) to Below Normal while you play.",
            "One less busy process competing with the game.",
            "Alt-Tabbing, the Start menu and file windows can feel a little slower during the session." } },
        { "smt", {
            "Uses Windows CPU sets so the game prefers one logical CPU per physical core, instead of placing two game threads on the same core.",
            "Each game thread gets a whole core, which can help games with a few heavy threads.",
            "Halves the logical CPUs the game prefers - games that scale to many threads can lose FPS. Test it per game." } },
        { "unpark", {
            "Sets the power plan's minimum unparked cores to 100% while you play.",
            "Cores never have to wake up from parking, which can remove small hitches on some Intel and non-X3D chips.",
            "More power and heat. On X3D chips it breaks the V-Cache core parking that keeps games on the right CCD, so it's locked there." } },
        { "cstate", {
            "Turns off deep CPU idle states (processor idle disable) in the power plan while you play.",
            "Cores respond instantly instead of waking from deep sleep, which can shave latency.",
            "Much more heat and power, and fewer boost clocks on chips that are limited by temperature. Locked on X3D chips." } },
        { "maxboost", {
            "Sets the power plan's boost mode to Aggressive and the energy performance preference to 0 while you play.",
            "The CPU boosts sooner and holds higher clocks.",
            "More power and heat. Locked on X3D chips because it interferes with AMD's V-Cache scheduling." } },
        { "standby", {
            "Clears the Windows standby memory list once when a game starts (skipped with 48 GB or more).",
            "Frees cached memory right before the game loads, which can help on 16 GB systems.",
            "Files that were cached must be read from disk again, so the first loads can be a little slower." } },
        { "ram_cleaner", {
            "Clears the standby list every few minutes during play, for games whose profile sets ramcleanup (twice as often with 16 GB).",
            "Helps games known for building up standby memory and stuttering after long sessions on 16-32 GB PCs.",
            "The clear itself can cause a small hitch, and useful cache is thrown away too. Only turn it on for games that need it." } },
        { "prefetch", {
            "Pauses the SysMain (Superfetch) service while you play and starts it again afterwards.",
            "No background prefetching reading your disk mid-game.",
            "Apps may open a bit slower right after the game, until SysMain warms up again." } },
        { "ws_trim", {
            "A minute after the game starts, trims the memory of background apps once, so Windows can page out what they don't use.",
            "Frees RAM for the game on systems that are short on memory.",
            "Those apps page memory back in when you use them, which can hitch them - or the game, if it's on the same slow disk." } },
        { "services", {
            "Pauses Windows Search, telemetry (DiagTrack) and the services in your pause_services list while you play, and starts them again afterwards.",
            "No indexing or telemetry uploads in the middle of a match.",
            "Windows search results can be briefly out of date after the session." } },
        { "updates", {
            "Stops the Windows Update services while you play and starts them again afterwards.",
            "No surprise update downloads eating bandwidth or disk time mid-game.",
            "Updates wait until you're done. Windows sometimes restarts them itself; they're paused again on the next check." } },
        { "close_apps", {
            "Closes the apps on your close lists (in profiles.ini) when a game starts, and reopens them afterwards if reopen_closed is on.",
            "Frees memory and CPU from apps you don't need while playing.",
            "Anything unsaved in those apps can be lost. Launchers a game needs are protected by its keep list." } },
        { "dvr", {
            "Turns off Game DVR background capture (GameDVR_Enabled and AppCaptureEnabled) while you play. Game Bar itself stays installed, so AMD's V-Cache game detection keeps working.",
            "Removes the capture hooks that can cost a few FPS and add frame-time spikes.",
            "Game Bar's 'Record what happened' and screenshots don't work during the session." } },
        { "visual", {
            "Turns off window and taskbar animations while you play (only until you sign out, even if the app crashes).",
            "Slightly less work for the desktop compositor when you Alt-Tab or use a second screen.",
            "Windows looks less smooth during the session. The gain is tiny on a modern GPU." } },
        { "fso", {
            "Sets Windows' 'Disable fullscreen optimizations' compatibility option on each game's exe. Windows reads it when the game starts, so it applies from the next launch.",
            "Games in fullscreen use true exclusive fullscreen, which can lower latency in some older DirectX 9-11 games.",
            "Alt-Tab is slower and overlays can break. Newer games often run better with it on. It stays set until you turn this tweak off." } },
        { "audio", {
            "Raises the Windows audio engine (audiodg.exe) to High priority while you play.",
            "Fewer audio crackles and pops when the CPU is busy.",
            "Rarely needed on a fast CPU. Needs admin." } },
        { "defender", {
            "Adds the game's folder to Microsoft Defender's exclusions while you play and removes it when the game closes.",
            "Defender doesn't scan files the game reads, which can reduce loading hitches.",
            "That folder isn't scanned for malware while you play. That's a real security trade-off, so it isn't in any preset." } },
        { "gpu_lock", {
            "Sets Windows' graphics preference for each game to your dedicated GPU (Settings > Display > Graphics), from the next launch.",
            "Games never land on the integrated graphics by mistake.",
            "None for gaming. Only matters when you have two GPUs." } },
        { "vendor_apps", {
            "Adds your GPU maker's helper apps (Radeon Software, the NVIDIA app and overlay, or Intel Graphics Software) to the low-priority list.",
            "Their background work competes less with the game.",
            "Their overlays and instant replay can respond a bit slower during the session." } },
        { "dwm", {
            "Raises the desktop window manager (dwm.exe) to High priority while you play.",
            "Can smooth borderless-windowed games and multi-monitor setups, since DWM composites every frame.",
            "Small gain, sometimes none. Needs admin." } },
        { "power_plan", {
            "Switches to the High performance power plan while you play and back to your plan afterwards. On dual-CCD X3D chips the Balanced plan is kept.",
            "Higher minimum clocks and faster boost response.",
            "More power and heat. On X3D chips it can break V-Cache core parking, so it's skipped there automatically." } },
        { "soft_pin", {
            "Core pinning uses Windows CPU sets instead of a hard affinity lock: the game is steered to the best cores (P-cores or the V-Cache CCD) "
            "rather than locked to them. Turned on automatically when Intel APO is installed. A game can also use it with cores = Prefer in profiles.ini.",
            "Games that use many threads - common on Intel chips with lots of E-cores - aren't starved when the best cores are busy. "
            "Works alongside Intel APO instead of fighting it.",
            "A weaker guarantee than hard pinning: threads can land on slower cores more often, so 1% lows can be a little worse in lightly threaded games. "
            "How strictly Windows follows CPU sets varies between Windows versions - test it per game." } },
        { "bg_eco", {
            "Puts background apps (the same list as Background apps low) into Windows efficiency mode while you play - the same thing Task Manager's "
            "'Efficiency mode' does - and takes them out again afterwards.",
            "Windows runs them at low clocks and, on Intel hybrid chips, keeps them on the E-cores (Intel Thread Director uses this hint). "
            "Softer than forcing their affinity, and it saves some power and heat.",
            "Those apps respond noticeably slower while you play - a browser or Discord on a second screen can feel sluggish, and voice apps can "
            "occasionally stutter." } },
        { "power_mode", {
            "On laptops, switches Windows' power mode (Settings > System > Power) to Best performance while you play, and puts your mode back afterwards. "
            "This is separate from the power plan.",
            "Laptops default to 'Balanced' or 'Best power efficiency', which hold the CPU and GPU back. Best performance lets them boost fully.",
            "More heat, fan noise and battery drain. Only available on PCs with a battery, and not on X3D chips." } },
        { "ecoqos", {
            "Tells Windows never to put the game in efficiency mode (EcoQoS power throttling).",
            "Windows can't slow the game down when it's in the background or on a second screen.",
            "Slightly more power use when the game is in the background. Not used for anti-cheat games." } },
    };
    static const Details none = { "", "", "" };
    auto it = d.find(id);
    return it == d.end() ? none : it->second;
}

const std::vector<const char*>& Categories() {
    static const std::vector<const char*> c = { "CPU", "Memory", "System", "GPU", "Power" };
    return c;
}

const Tweak* Find(const std::string& id) {
    for (auto& t : All()) if (id == t.id) return &t;
    return nullptr;
}

const std::vector<std::string>& BuiltIns() {
    static const std::vector<std::string> b = { "Safe", "Balanced", "Aggressive" };
    return b;
}

bool IsBuiltIn(const std::string& preset) {
    for (auto& b : BuiltIns()) if (b == preset) return true;
    return false;
}

std::set<std::string> PresetTweaks(const AppData& d, const std::string& preset) {
    std::set<std::string> on;
    int tier = preset == "Safe" ? 0 : preset == "Balanced" ? 1 : preset == "Aggressive" ? 2 : -1;
    if (tier >= 0) {
        for (auto& t : All()) if (t.tier <= tier) on.insert(t.id);
        return on;
    }
    auto it = d.tweakPresets.find(preset);
    if (it != d.tweakPresets.end())
        for (auto& id : it->second) if (Find(id)) on.insert(id);
    return on;
}

std::string Unavailable(const std::string& id, const SystemInfo& sys, const AppData& d) {
    bool x3d = sys.layout == CpuLayout::DualX3D || sys.layout == CpuLayout::SingleX3D;
    if (id == "pinning" && !sys.canPin) return "Needs a multi-die or hybrid CPU";
    if (id == "bg_affinity" && (!sys.canPin || !sys.otherMask)) return "Needs a multi-die or hybrid CPU";
    if (id == "bg_affinity" && d.settings.backgroundCores == "off") return "Off in profiles.ini";
    if (id == "soft_pin" && !sys.canPin) return "Needs a multi-die or hybrid CPU";
    if (id == "power_mode" && !sys.hasBattery) return "Laptops only";
    if (id == "power_mode" && x3d) return "Off on X3D - fights V-Cache parking";
    if ((id == "unpark" || id == "cstate" || id == "maxboost") && x3d) return "Off on X3D - fights V-Cache parking";
    if (id == "smt" && sys.threads <= sys.cores) return "Needs SMT / Hyper-Threading";
    if (id == "gpu_lock" && sys.gpus.size() < 2) return "Needs two GPUs";
    if (id == "gpu_lock" && !d.settings.forceGpu) return "Off in profiles.ini";
    if (id == "power_plan" && d.settings.powerPlan == "off") return "Off in profiles.ini";
    if (id == "power_plan" && d.settings.powerPlan == "auto" && sys.layout == CpuLayout::DualX3D) return "X3D keeps Balanced";
    if (id == "updates" && !d.settings.pauseUpdates) return "Off in profiles.ini";
    if (id == "standby" && sys.ramGB >= 48) return "Not needed with 48 GB+";
    if (id == "ram_cleaner" && sys.ramGB >= 48) return "Not needed with 48 GB+";
    if (id == "vendor_apps" && !d.settings.vendorApps) return "Off in profiles.ini";
    if (id == "standby" && !d.settings.cleanupOnLaunch) return "Off in profiles.ini";
    return "";
}

bool PresetExists(const AppData& d, const std::string& preset) { return IsBuiltIn(preset) || d.tweakPresets.count(preset) > 0; }

std::string PresetFor(const AppData& d, const GameProfile* p) {
    if (!p || p->tweaks.empty()) return d.tweakPreset;
    if (p->tweaks == "Custom" || PresetExists(d, p->tweaks)) return p->tweaks;
    return d.tweakPreset;   // its preset was deleted or renamed
}

std::set<std::string> ChosenFor(const AppData& d, const GameProfile* p) {
    std::string preset = PresetFor(d, p);
    if (preset != "Custom") return PresetTweaks(d, preset);
    std::set<std::string> on;
    for (auto& id : p->tweakIds) if (Find(id)) on.insert(id);
    return on;
}

std::set<std::string> Active(const AppData& d, const SystemInfo& sys, const GameProfile* p) {
    std::set<std::string> on;
    for (auto& id : ChosenFor(d, p))
        if (Unavailable(id, sys, d).empty()) on.insert(id);
    if (sys.apo && sys.canPin) on.insert("soft_pin");   // never fight Intel APO with hard pinning
    return on;
}

std::string ExportJson(const std::string& name, const std::set<std::string>& on) {
    Json j = Json::Obj();
    j.obj["ProjectOptMTweaks"] = Json::Num(1);
    j.obj["Name"] = Json::Str(name);
    j.obj["On"] = Json::StrList(std::vector<std::string>(on.begin(), on.end()));
    return j.Dump() + "\r\n";
}

bool ImportJson(const std::string& text, std::string& name, std::set<std::string>& on) {
    Json j = Json::Parse(text);
    if (j.type != Json::Object || j["ProjectOptMTweaks"].type != Json::Number) return false;
    name = j["Name"].AsString("Imported");
    on.clear();
    for (auto& id : j["On"].AsStrings()) if (Find(id)) on.insert(id);
    return true;
}

}  // namespace tweakset
