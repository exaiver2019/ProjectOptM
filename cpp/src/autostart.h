// The admin task: a Task Scheduler task that runs Project OptM with "highest privileges".
// Windows only asks for admin once, when you turn it on in Settings. After that:
//  - "Open without the admin prompt": opening the app starts it through the task (no UAC prompt)
//  - "Start with Windows": the same task also runs at sign-in (hidden in the tray)
#pragma once
#include <string>

namespace autostart {

struct State {
    bool exists = false;        // the task is there (opening needs no prompt)
    bool atSignIn = false;      // ...and it starts the app when you sign in
    std::wstring exe;           // the exe it starts
};

State Query();                                                     // this Windows user's task
bool Set(const std::wstring& exe, bool atSignIn, std::string& error);   // create or update (needs admin - we are)
bool Remove(std::string& error);
bool Run();                     // start it now (works without admin - that's the point)

}  // namespace autostart
