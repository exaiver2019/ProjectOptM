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
    std::vector<std::string> log_;
};

// Helpers shared with the rest of the app
namespace net {
bool Get(const std::string& url, std::string& body, std::string& error, int timeoutSec = 15);
}
