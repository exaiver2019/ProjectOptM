// In-game FPS overlay: a small click-through window kept above the game (like Game Bar widgets).
// Nothing is loaded into the game, so it's safe with anti-cheat - but it only shows over windowed,
// borderless and "fullscreen optimizations" games, not old exclusive fullscreen.
// Drawn with per-pixel alpha (GDI+ into a layered window), so the background can be see-through
// while the text stays solid. Where it sits is chosen on the Overlay page (a picture of your screen).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

struct OverlayStat {
    std::wstring label, value;     // "GPU", "97%  63°C"
};

struct OverlayContent {
    bool hasFps = false;
    double fps = 0;
    std::vector<OverlayStat> stats;     // two per row under the FPS
    bool showGraph = false;
    std::vector<double> graph;          // worst frametime per column, oldest first (ms)
    std::vector<uint8_t> marks;         // 1 = a stutter in that column (drawn red)
    COLORREF accent = RGB(79, 139, 255);
    int opacity = 85;                   // background, percent (0 = text only)
};

class Overlay {
public:
    ~Overlay() { Destroy(); }
    bool Create(HINSTANCE inst);
    void Destroy();
    // at (fx, fy) - 0..1 across the screen `anchor` is on (0,0 top-left ... 1,1 bottom-right)
    void Show(HWND anchor, double fx, double fy, float scale, const OverlayContent& content);
    void Hide();
    bool Visible() const { return visible_; }

    // Size and placement, shared with the Overlay page's screen preview
    static SIZE SizeFor(size_t stats, bool graph, float scale);
    static int Margin(float scale);                                   // gap to the screen edge
    static POINT Place(const RECT& screen, SIZE size, double fx, double fy, float scale);

private:
    void Render(int x, int y);

    HWND hwnd_ = nullptr;
    ULONG_PTR gdiplus_ = 0;
    bool visible_ = false;
    OverlayContent c_;
    float s_ = 1;
    uint64_t lastTopmost_ = 0;
};
