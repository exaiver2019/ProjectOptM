#include "share.h"
#include "json.h"
#include "util.h"
#include <algorithm>

namespace share {

namespace {
const char* kPrefix = "OPTM-GAME1:";
const char* kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64(const std::string& in) {
    std::string out;
    int val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c; bits += 8;
        while (bits >= 0) { out.push_back(kB64[(val >> bits) & 0x3F]); bits -= 6; }
    }
    if (bits > -6) out.push_back(kB64[((val << 8) >> (bits + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

bool Unbase64(const std::string& in, std::string& out) {
    out.clear();
    int val = 0, bits = -8;
    for (char c : in) {
        if (c == '=' || c == ' ' || c == '\r' || c == '\n' || c == '\t') continue;
        const char* p = strchr(kB64, c);
        if (!p || !c) return false;
        val = (val << 6) + (int)(p - kB64); bits += 6;
        if (bits >= 0) { out.push_back((char)((val >> bits) & 0xFF)); bits -= 8; }
    }
    return !out.empty();
}

// names in a code become process names and profile values: keep them to plain characters
std::string Clean(const std::string& s, size_t max) {
    std::string o;
    for (char c : s) if ((unsigned char)c >= 32 && c != '[' && c != ']' && c != '=' && c != ';') o += c;
    o = util::Trim(o);
    if (o.size() > max) o.resize(max);
    return o;
}
std::vector<std::string> CleanList(const std::vector<std::string>& v) {
    std::vector<std::string> o;
    for (auto& s : v) { std::string c = Clean(s, 64); if (!c.empty() && c.find(',') == std::string::npos && o.size() < 20) o.push_back(c); }
    return o;
}
}  // namespace

std::string Encode(const GameProfile& p) {
    Json j = Json::Obj();
    j.obj["Name"] = Json::Str(p.name);
    j.obj["Exe"] = Json::StrList(p.exes);
    j.obj["AntiCheat"] = Json::Boolean(p.antiCheat);
    j.obj["Priority"] = Json::Str(p.priority);
    j.obj["Cores"] = Json::Str(p.softPin && p.cores == "Best" ? "Prefer" : p.cores);
    j.obj["RamCleanup"] = Json::Num(p.ramCleanupMins);
    j.obj["Boost"] = Json::StrList(p.boost);
    j.obj["Keep"] = Json::StrList(p.keep);
    j.obj["Close"] = Json::StrList(p.close);
    j.obj["LaunchPriority"] = Json::Str(p.launchPriority);
    j.obj["EcoQosOff"] = Json::Boolean(p.ecoQosOff);
    j.obj["Tweaks"] = Json::Str(p.tweaks);
    if (p.tweaks == "Custom") j.obj["TweakIds"] = Json::StrList(p.tweakIds);
    // Dump() is laid out for people; a code is shorter without the spaces and line breaks
    std::string text = j.Dump(), compact;
    bool inStr = false;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (inStr) { compact += c; if (c == '\\' && i + 1 < text.size()) compact += text[++i]; else if (c == '"') inStr = false; }
        else if (c == '"') { inStr = true; compact += c; }
        else if (c != ' ' && c != '\r' && c != '\n' && c != '\t') compact += c;
    }
    return kPrefix + Base64(compact);
}

bool Decode(const std::string& codeIn, GameProfile& out, std::string& error) {
    std::string code = util::Trim(codeIn);
    size_t at = code.find(kPrefix);
    if (at == std::string::npos) { error = "That isn't a Project OptM game code (they start with OPTM-GAME1:)."; return false; }
    std::string text;
    if (!Unbase64(code.substr(at + strlen(kPrefix)), text)) { error = "The code is damaged - copy all of it and try again."; return false; }
    Json j = Json::Parse(text);
    if (j.type != Json::Object) { error = "The code is damaged - copy all of it and try again."; return false; }
    GameProfile p;
    p.name = Clean(j["Name"].AsString(), 60);
    p.exes = CleanList(j["Exe"].AsStrings());
    for (auto& e : p.exes) e = util::StripExe(e);
    if (p.name.empty() || p.exes.empty()) { error = "The code has no game name or exe."; return false; }
    p.antiCheat = j["AntiCheat"].AsBool(false);
    auto pick = [](const std::string& v, std::initializer_list<const char*> ok, const char* def) {
        for (auto* o : ok) if (v == o) return v;
        return std::string(def);
    };
    p.priority = pick(j["Priority"].AsString(), { "Normal", "AboveNormal", "High" }, "AboveNormal");
    std::string cores = pick(j["Cores"].AsString(), { "Best", "Other", "All", "Prefer" }, "Best");
    p.softPin = cores == "Prefer";
    p.cores = cores == "Prefer" ? "Best" : cores;
    p.ramCleanupMins = j["RamCleanup"].type == Json::Number ? std::clamp((int)j["RamCleanup"].num, 0, 60) : 0;
    p.boost = CleanList(j["Boost"].AsStrings());
    p.keep = CleanList(j["Keep"].AsStrings());
    p.close = CleanList(j["Close"].AsStrings());
    p.launchPriority = pick(j["LaunchPriority"].AsString(), { "", "Normal", "AboveNormal", "High" }, "");
    p.ecoQosOff = j["EcoQosOff"].AsBool(true);
    p.tweaks = Clean(j["Tweaks"].AsString(), 40);
    if (p.tweaks == "Custom") p.tweakIds = CleanList(j["TweakIds"].AsStrings());
    out = p;
    return true;
}

}  // namespace share
