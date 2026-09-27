// In-game FPS overlay: a small click-through window kept above the game (like Game Bar widgets).
// Nothing is loaded into the game, so it's safe with anti-cheat - but it only shows over windowed,
// borderless and "fullscreen optimizations" games, not old exclusive fullscreen.
#pragma once
#include <cstdint>
#include <vector>
#include <windows.h>

struct OverlayContent {
    bool hasFps = false;
    double fps = 0, low1 = 0, frametime = 0;
    std::vector<double> graph;      // worst frametime per column, oldest first (ms)
    bool showGraph = true;
    COLORREF accent = RGB(79, 139, 255);
};

class Overlay {
public:
    ~Overlay() { Destroy(); }
    bool Create(HINSTANCE inst);
    void Destroy();
    // shown in a corner (0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right) of the game's monitor
    void Show(HWND game, int corner, float zoom, const OverlayContent& content);
    void Hide();
    bool Visible() const { return visible_; }
    RECT Rect() const { return placed_; }

private:
    static LRESULT CALLBACK Proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
    void Paint(HDC dc, int w, int h);
    void Fonts(float scale);

    HWND hwnd_ = nullptr;
    bool visible_ = false;
    OverlayContent c_;
    float scale_ = 0;
    HFONT big_ = nullptr, small_ = nullptr;
    RECT placed_ = {};
    uint64_t lastTopmost_ = 0;
};
