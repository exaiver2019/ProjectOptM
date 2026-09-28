// Screens: refresh rates, and "is a video playing on the other screen while you game?"
// (with screens at different refresh rates, a video on one while gaming on the other is a known cause of stutter).
// Read-only: window positions and Windows' per-app audio meters. Nothing is changed.
#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace screens {

struct Screen { HMONITOR mon; int hz; RECT rect; bool primary; };
std::vector<Screen> All();
int RefreshOf(HWND w);                       // refresh rate of the screen a window is on (0 = unknown)

// A browser or video player that's making sound and has a window on a different screen than `game`.
// Fills app ("chrome") and that screen's refresh rate. Needs COM (it initializes it for the call).
bool MediaOnOtherScreen(HWND game, std::string& app, int& hz);

}  // namespace screens
