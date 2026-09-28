// Self-updater: checks GitHub releases, downloads ProjectOptM.exe, verifies it and swaps it in.
// Network work runs on a background thread; the app polls for results.
#pragma once
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Updater {
public:
    enum State { Idle, Checking, UpToDate, Available, Downloading, Ready, Failed };

    ~Updater();
    bool Enabled() const;
    void Check(bool manual);           // starts a background check
    void Install();                    // starts the download + swap (after the user agreed)

    // Main thread
    State GetState() const;
    std::string Status() const;        // one line for the About card
    std::string Version() const;       // newer version on offer
    std::string Notes() const;
    std::vector<std::string> TakeLog();   // lines for the activity log

    // "stable": only full releases (releases/latest). "experimental": pre-releases too (the newest of all).
    void SetChannel(const std::string& channel);
    static std::string CurrentTag();      // this build as a tag: "2.1.0", "2.1.1-experimental.3", "2.1.1-experimental.0" (local)
    // Version order: numbers first, then stable > experimental > unstable, then the pre-release number.
    // "v2.1.1" > "v2.1.1-experimental.3" > "v2.1.1-experimental.1" > "2.1.0"
    static bool Newer(const std::string& tag, const std::string& current);

private:
    void DoCheck(bool manual);
    void DoInstall();
    void Set(State s, const std::string& status);
    void Say(const std::string& line);
    void Join();

    mutable std::mutex mu_;
    std::thread worker_;
    State state_ = Idle;
    std::string status_, version_, url_, notes_, digest_;
    std::string channel_ = "stable";
    bool prerelease_ = false;          // the update on offer is a pre-release
    std::vector<std::string> log_;
};

// Helpers shared with the rest of the app
namespace net {
bool Get(const std::string& url, std::string& body, std::string& error, int timeoutSec = 15);
}
