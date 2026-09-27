// What the session history says: game tests (which CCD, does a tweak help) and crash patterns.
// Pure functions over AppData::history - nothing here touches the system.
#pragma once
#include <string>
#include <vector>
#include "data.h"

namespace insights {

// A test runs over the next sessions of one game, alternating two setups:
//   "ccd"          V-Cache CCD vs the other (frequency) CCD - dual-CCD X3D only
//   "ab:<tweak>"   a tweak on vs off
struct Test {
    std::string id;            // "ccd" / "ab:timer"
    std::string a, b;          // session variant labels: "ccd:vcache" / "ccd:freq", "ab:timer:on" / "ab:timer:off"
    std::string aName, bName;  // "V-Cache CCD" / "Frequency CCD", "Timer on" / "Timer off"
    int need = 2;              // sessions of each that count before there's a result
    std::string since;         // only sessions from this date on count ("yyyy-MM-dd HH:mm", from "ccd|<date>")
};
Test Describe(const std::string& id);          // id may carry its start: "ccd|2026-09-27 14:05"

bool Counts(const Session& s);            // long enough with FPS to count for a test (5+ min, 1+ min of FPS)

struct Stats {
    int n = 0;
    double avg = 0, low = 0, low01 = 0, stutters = -1;   // means; stutters per minute (-1 = unknown)
};
Stats Of(const std::vector<Session>& h, const std::string& game, const std::string& variant, const std::string& since = "");

// Which setup the next session of this game uses ("" = the test is finished)
std::string Next(const Test& t, const std::vector<Session>& h, const std::string& game);
bool Finished(const Test& t, const std::vector<Session>& h, const std::string& game);

// The result in words, and which one won ("" = no clear difference)
std::string Verdict(const Test& t, const Stats& a, const Stats& b, std::string& winner);

// "Crashed in 3 of 5 sessions with Aggressive tweaks, 0 of 8 with Safe" ("" = nothing to say)
std::string CrashPattern(const std::vector<Session>& h, const std::string& game);

// Two sessions side by side: one line per number that both have ("Avg FPS", "142", "151", "+9")
struct Row { std::string what, a, b, diff; int better = 0; };   // better: 1 = b is better, -1 = a is, 0 = same
std::vector<Row> Compare(const Session& a, const Session& b);

}  // namespace insights
