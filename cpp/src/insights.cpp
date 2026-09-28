#include "insights.h"
#include "tweakset.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace insights {

namespace {
std::string Fmt(const char* f, double v) { char b[32]; snprintf(b, sizeof(b), f, v); return b; }
std::string Signed(double v, const char* f = "%+.0f") { return Fmt(f, v); }
}  // namespace

Test Describe(const std::string& full) {
    Test t;
    size_t bar = full.find('|');
    std::string id = full.substr(0, bar);
    if (bar != std::string::npos) t.since = full.substr(bar + 1);
    t.id = id;
    if (id == "ccd") {
        t.a = "ccd:vcache"; t.b = "ccd:freq";
        t.aName = "V-Cache CCD"; t.bName = "Frequency CCD";
        t.need = 2;
    } else if (id.rfind("ab:", 0) == 0) {
        std::string tw = id.substr(3);
        const tweakset::Tweak* k = tweakset::Find(tw);
        std::string name = k ? k->name : tw;
        t.a = id + ":on"; t.b = id + ":off";
        t.aName = name + " on"; t.bName = name + " off";
        t.need = 3;
    }
    return t;
}

bool Counts(const Session& s) { return s.minutes >= 5 && s.avgFps > 0 && (s.fpsSeconds <= 0 || s.fpsSeconds >= 60); }

Stats Of(const std::vector<Session>& h, const std::string& game, const std::string& variant, const std::string& since) {
    Stats st;
    double sa = 0, sl = 0, s01 = 0, ss = 0;
    int n01 = 0, ns = 0;
    for (auto& s : h) {
        if (s.game != game || s.variant != variant || !Counts(s) || s.date < since) continue;
        st.n++;
        sa += s.avgFps; sl += s.low1;
        if (s.low01 > 0) { s01 += s.low01; n01++; }
        double spm = s.StuttersPerMin();
        if (spm >= 0) { ss += spm; ns++; }
    }
    if (st.n) { st.avg = sa / st.n; st.low = sl / st.n; }
    if (n01) st.low01 = s01 / n01;
    if (ns) st.stutters = ss / ns;
    return st;
}

std::string Next(const Test& t, const std::vector<Session>& h, const std::string& game) {
    if (t.a.empty()) return "";
    int na = Of(h, game, t.a, t.since).n, nb = Of(h, game, t.b, t.since).n;
    if (na >= t.need && nb >= t.need) return "";
    if (na >= t.need) return t.b;
    if (nb >= t.need) return t.a;
    return na <= nb ? t.a : t.b;   // alternate, A first
}

bool Finished(const Test& t, const std::vector<Session>& h, const std::string& game) {
    return !t.a.empty() && Next(t, h, game).empty();
}

std::string Verdict(const Test& t, const Stats& a, const Stats& b, std::string& winner) {
    winner.clear();
    if (!a.n || !b.n) return "Not enough sessions yet.";
    // 1% lows decide (smoothness), average FPS breaks a tie; under 3% is within the normal
    // difference between two sessions of the same game
    double dLow = b.low - a.low, pLow = a.low > 0 ? dLow / a.low * 100 : 0;
    double dAvg = b.avg - a.avg, pAvg = a.avg > 0 ? dAvg / a.avg * 100 : 0;
    std::string nums = t.bName + " vs " + t.aName + ": " + Signed(pAvg, "%+.1f") + "% average FPS, " + Signed(pLow, "%+.1f") + "% 1% lows";
    if (a.stutters >= 0 && b.stutters >= 0) nums += ", " + Fmt("%.1f", b.stutters) + " vs " + Fmt("%.1f", a.stutters) + " stutters/min";
    bool lowClear = std::fabs(pLow) >= 3, avgClear = std::fabs(pAvg) >= 3;
    if (!lowClear && !avgClear) return nums + ". No clear difference - keep what you have.";
    if (lowClear && avgClear && (pLow > 0) != (pAvg > 0))
        return nums + ". Mixed: " + (pAvg > 0 ? t.bName : t.aName) + " is faster on average, " + (pLow > 0 ? t.bName : t.aName) +
               " is smoother (better 1% lows) - pick the one that feels better, or keep what you have.";
    bool bWins = lowClear ? pLow > 0 : pAvg > 0;
    winner = bWins ? t.b : t.a;
    return nums + ". " + (bWins ? t.bName : t.aName) + " ran better for this game.";
}

std::string CrashPattern(const std::vector<Session>& h, const std::string& game) {
    std::map<std::string, std::pair<int, int>> byPreset;   // preset -> crashes, sessions
    int crashes = 0;
    for (auto& s : h) {
        if (s.game != game || s.preset.empty()) continue;
        bool crash = s.exit.rfind("crash", 0) == 0 || s.exit == "hang";
        auto& c = byPreset[s.preset];
        c.second++;
        if (crash) { c.first++; crashes++; }
    }
    if (crashes < 2) return "";   // one crash says nothing
    std::vector<std::pair<std::string, std::pair<int, int>>> v(byPreset.begin(), byPreset.end());
    std::sort(v.begin(), v.end(), [](auto& x, auto& y) {
        return x.second.first * y.second.second > y.second.first * x.second.second;   // highest crash rate first
    });
    std::string out = "Crashes by tweak preset: ";
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out += ", ";
        out += std::to_string(v[i].second.first) + " of " + std::to_string(v[i].second.second) + " with " + v[i].first;
    }
    if (v.size() >= 2 && v[0].second.first >= 2 && v.back().second.first == 0 && v.back().second.second >= 2)
        out += " - " + v[0].first + " may not suit this game.";
    return out;
}

Heat HeatOf(const std::vector<HeatSample>& samples) {
    Heat h;
    std::vector<float> hot, cool;
    for (auto& s : samples) {
        if (s.temp < 0) continue;
        h.maxTemp = std::max(h.maxTemp, (double)s.temp);
        if (s.temp >= 85) h.hotSeconds++;
        if (s.gpu < 90 || s.fps <= 0) continue;   // only when the GPU was the limit
        if (s.temp >= 85) hot.push_back(s.fps); else if (s.temp < 80) cool.push_back(s.fps);
    }
    if (hot.size() >= 30 && cool.size() >= 30) {
        auto median = [](std::vector<float> v) { std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end()); return (double)v[v.size() / 2]; };
        double mh = median(hot), mc = median(cool);
        double drop = mc > 0 ? (mc - mh) / mc * 100 : 0;
        if (drop >= 10) h.drop = std::round(drop);
    }
    return h;
}

std::string HeatNote(const Session& s) {
    if (s.hotSeconds < 60) return "";
    std::string t = "The GPU ran at 85 C or more for " + std::to_string(s.hotSeconds / 60) + " min" +
                    (s.gpuTempMax > 0 ? " (up to " + Fmt("%.0f", s.gpuTempMax) + " C)" : "");
    if (s.heatDrop > 0)
        return t + ", and FPS was " + Fmt("%.0f", s.heatDrop) + "% lower while it was that hot at full load - it was likely "
               "slowing itself down to cool off. Check the case airflow, dust and the GPU fan curve.";
    return t + ". FPS didn't drop because of it, but better airflow keeps it from throttling in longer sessions.";
}

std::string CapAdvice(const Session& s, const std::string& gpuVendor, int& cap) {
    cap = 0;
    if (s.hz < 50 || s.avgFps <= 0 || (s.fpsSeconds > 0 && s.fpsSeconds < 60)) return "";
    std::string why;
    if (s.avgFps > s.hz * 1.05) {
        cap = s.hz >= 100 ? s.hz - 3 : s.hz - 2;
        why = "Your FPS (avg " + Fmt("%.0f", s.avgFps) + ") goes past your " + std::to_string(s.hz) + " Hz screen, so the extra frames are never "
              "shown. A cap at " + std::to_string(cap) + " FPS keeps FreeSync / G-Sync in range and cuts input lag, heat and fan noise.";
    } else if (s.fps5 >= 60 && s.fps5 < s.avgFps * 0.75) {
        cap = std::min((int)(s.fps5 / 5) * 5, s.hz >= 100 ? s.hz - 3 : s.hz - 2);
        // not worth it when it would take most of your FPS away (then it's stutters to fix, not pacing)
        if (cap < 60 || cap >= s.avgFps * 0.9 || cap < s.avgFps * 0.6) { cap = 0; return ""; }
        why = "FPS swings a lot: avg " + Fmt("%.0f", s.avgFps) + ", but 1 frame in 20 is slower than " + Fmt("%.0f", s.fps5) +
              " FPS. A cap around " + std::to_string(cap) + " would feel steadier - frames arrive evenly - at the cost of the peaks.";
    } else {
        return "";
    }
    std::string where = gpuVendor == "AMD" ? "AMD Software > Gaming > the game > Radeon Chill, with min and max both at " + std::to_string(cap)
                      : gpuVendor == "NVIDIA" ? "NVIDIA app > Graphics > the game > Max Frame Rate"
                      : "your graphics driver's frame limiter";
    return why + " Use the game's own frame limit if it has one, otherwise " + where + ".";
}

std::vector<Row> Compare(const Session& a, const Session& b) {
    std::vector<Row> rows;
    auto add = [&](const char* what, double va, double vb, const char* f, bool higherBetter, double same) {
        if (va <= 0 && vb <= 0) return;
        Row r;
        r.what = what;
        r.a = va > 0 ? Fmt(f, va) : "--";
        r.b = vb > 0 ? Fmt(f, vb) : "--";
        if (va > 0 && vb > 0) {
            double d = vb - va;
            r.diff = Fmt(std::string(f) == "%.1f" ? "%+.1f" : "%+.0f", d);
            r.better = std::fabs(d) < same ? 0 : ((d > 0) == higherBetter ? 1 : -1);
        }
        rows.push_back(r);
    };
    add("Average FPS", a.avgFps, b.avgFps, "%.0f", true, std::max(1.0, a.avgFps * 0.02));
    add("1% low", a.low1, b.low1, "%.0f", true, std::max(1.0, a.low1 * 0.03));
    add("0.1% low", a.low01, b.low01, "%.0f", true, std::max(1.0, a.low01 * 0.05));
    double sa = a.StuttersPerMin(), sb = b.StuttersPerMin();
    if (sa >= 0 || sb >= 0) {
        Row r;
        r.what = "Stutters / min";
        r.a = sa >= 0 ? Fmt("%.1f", sa) : "--";
        r.b = sb >= 0 ? Fmt("%.1f", sb) : "--";
        if (sa >= 0 && sb >= 0) { r.diff = Fmt("%+.1f", sb - sa); r.better = std::fabs(sb - sa) < 0.3 ? 0 : (sb < sa ? 1 : -1); }
        rows.push_back(r);
    }
    add("GPU temp avg (C)", a.gpuTempAvg, b.gpuTempAvg, "%.0f", false, 2);
    add("GPU temp max (C)", a.gpuTempMax, b.gpuTempMax, "%.0f", false, 2);
    add("GPU at 85 C+ (min)", a.hotSeconds / 60.0, b.hotSeconds / 60.0, "%.0f", false, 1);
    add("CPU use (%)", a.cpuAvg, b.cpuAvg, "%.0f", false, 3);
    add("Video on other screen (min)", a.otherVideoSeconds / 60.0, b.otherVideoSeconds / 60.0, "%.0f", false, 1);
    add("Ping (ms)", a.pingAvg, b.pingAvg, "%.0f", false, 3);
    add("Played (min)", a.minutes, b.minutes, "%.0f", true, 1e9);   // never "better"
    auto text = [&](const char* what, const std::string& x, const std::string& y) {
        if (x.empty() && y.empty()) return;
        Row r; r.what = what; r.a = x.empty() ? "--" : x; r.b = y.empty() ? "--" : y;
        rows.push_back(r);
    };
    text("Tweaks", a.preset, b.preset);
    text("Cores", a.cores, b.cores);
    text("Ended", a.exit.empty() ? (a.version.empty() ? "" : "normally") : a.exit, b.exit.empty() ? (b.version.empty() ? "" : "normally") : b.exit);
    return rows;
}

}  // namespace insights
