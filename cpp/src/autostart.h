// Start with Windows: a Task Scheduler task that starts Project OptM in the tray when you sign in,
// with "run with highest privileges" - so it's already admin and Windows doesn't ask every time.
#pragma once
#include <string>

namespace autostart {

bool Enabled();                                  // the task exists for this user
std::wstring TaskCommand();                      // the exe the task starts ("" if none)
bool Enable(const std::wstring& exe, std::string& error);   // create or update (needs admin - we are)
bool Disable(std::string& error);

}  // namespace autostart
