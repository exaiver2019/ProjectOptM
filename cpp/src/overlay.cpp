#include "overlay.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
const wchar_t* kClass = L"ProjectOptMOverlay";
const COLORREF kBg = RGB(12, 13, 17), kText = RGB(232, 234, 240), kDim = RGB(140, 145, 160);
const COLORREF kGood = RGB(61, 220, 132), kSpike = RGB(245, 185, 66);
}

bool Overlay::Create(HINSTANCE inst) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = Proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClass;
    RegisterClassExW(&wc);
    // topmost, see-through for the mouse, never takes focus, not in the taskbar or Alt+Tab
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            kClass, L"Project OptM overlay", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr, inst, nullptr);
    if (!hwnd_) return false;
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, (LONG_PTR)this);
    SetLayeredWindowAttributes(hwnd_, 0, 242, LWA_ALPHA);   // barely see-through, so the game behind doesn't muddle the numbers
    return true;
}

void Overlay::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    if (big_) { DeleteObject(big_); big_ = nullptr; }
    if (small_) { DeleteObject(small_); small_ = nullptr; }
    visible_ = false;
}

void Overlay::Fonts(float scale) {
    if (scale == scale_ && big_) return;
    scale_ = scale;
    if (big_) DeleteObject(big_);
    if (small_) DeleteObject(small_);
    big_ = CreateFontW(-(int)std::lround(26 * scale), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    small_ = CreateFontW(-(int)std::lround(12 * scale), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

void Overlay::Show(HWND game, int corner, float zoom, const OverlayContent& content) {
    if (!hwnd_ || !game) return;
    c_ = content;
    float s = (GetDpiForWindow(game) / 96.0f) * zoom;
    Fonts(s);
    int w = (int)std::lround(172 * s), h = (int)std::lround((c_.showGraph ? 76 : 54) * s), m = (int)std::lround(14 * s);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(game, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& r = mi.rcMonitor;   // the whole screen - games cover the taskbar
    int x = (corner == 1 || corner == 3) ? r.right - w - m : r.left + m;
    int y = (corner >= 2) ? r.bottom - h - m : r.top + m;
    RECT want = { x, y, x + w, y + h };
    if (!EqualRect(&want, &placed_)) {
        placed_ = want;
        SetWindowPos(hwnd_, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
        int rad = (int)std::lround(10 * s);
        SetWindowRgn(hwnd_, CreateRoundRectRgn(0, 0, w + 1, h + 1, rad, rad), TRUE);
    }
    // games and other overlays go topmost too - stay above them
    uint64_t now = GetTickCount64();
    if (now - lastTopmost_ > 1000) {
        lastTopmost_ = now;
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (!visible_) { ShowWindow(hwnd_, SW_SHOWNOACTIVATE); visible_ = true; }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void Overlay::Hide() {
    if (hwnd_ && visible_) ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
}

void Overlay::Paint(HDC dc, int w, int h) {
    float s = scale_ > 0 ? scale_ : 1;
    auto px = [&](float v) { return (int)std::lround(v * s); };
    HBRUSH bg = CreateSolidBrush(kBg);
    RECT all = { 0, 0, w, h };
    FillRect(dc, &all, bg);
    DeleteObject(bg);
    SetBkMode(dc, TRANSPARENT);

    // big FPS + "FPS"
    wchar_t b[64];
    if (c_.hasFps) swprintf(b, 64, L"%d", (int)std::lround(c_.fps)); else wcscpy_s(b, L"--");
    HGDIOBJ old = SelectObject(dc, big_);
    SetTextColor(dc, c_.accent);
    RECT rb = { px(12), px(6), w, px(40) };
    DrawTextW(dc, b, -1, &rb, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    SIZE bs; GetTextExtentPoint32W(dc, b, (int)wcslen(b), &bs);
    SelectObject(dc, small_);
    SetTextColor(dc, kDim);
    RECT rf = { px(12) + bs.cx + px(4), px(19), w, px(36) };
    DrawTextW(dc, L"FPS", -1, &rf, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    // right column: 1% low and frametime
    SetTextColor(dc, kText);
    if (c_.hasFps && c_.low1 > 0) swprintf(b, 64, L"1%% low %d", (int)std::lround(c_.low1)); else wcscpy_s(b, L"1% low --");
    RECT r1 = { 0, px(9), w - px(12), px(24) };
    DrawTextW(dc, b, -1, &r1, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    if (c_.hasFps) swprintf(b, 64, L"%.1f ms", c_.frametime); else wcscpy_s(b, L"-- ms");
    SetTextColor(dc, kDim);
    RECT r2 = { 0, px(25), w - px(12), px(40) };
    DrawTextW(dc, b, -1, &r2, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);

    // mini frametime graph: one bar per column, spikes in amber
    if (c_.showGraph) {
        int gx = px(12), gw = w - px(24), gy = h - px(28), gh = px(20);
        HBRUSH track = CreateSolidBrush(RGB(24, 26, 32));
        RECT gr = { gx, gy, gx + gw, gy + gh };
        FillRect(dc, &gr, track);
        DeleteObject(track);
        const auto& g = c_.graph;
        if (!g.empty()) {
            std::vector<double> sorted;
            for (double v : g) if (v > 0) sorted.push_back(v);
            if (!sorted.empty()) {
                std::sort(sorted.begin(), sorted.end());
                double median = sorted[sorted.size() / 2];
                double top = std::max(sorted.back() * 1.15, median * 2.0);
                HBRUSH good = CreateSolidBrush(kGood), spike = CreateSolidBrush(kSpike);
                int n = (int)g.size();
                for (int i = 0; i < n; i++) {
                    if (g[i] <= 0) continue;
                    int x0 = gx + (int)((long long)gw * i / n), x1 = gx + (int)((long long)gw * (i + 1) / n);
                    if (x1 <= x0) x1 = x0 + 1;
                    int bh = std::max(1, (int)std::lround(gh * std::min(1.0, g[i] / top)));
                    RECT bar = { x0, gy + gh - bh, x1, gy + gh };
                    FillRect(dc, &bar, g[i] > median * 1.8 ? spike : good);
                }
                DeleteObject(good);
                DeleteObject(spike);
            }
        }
    }
}

LRESULT CALLBACK Overlay::Proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = (Overlay*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_NCHITTEST: return HTTRANSPARENT;          // clicks go to the game
        case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            RECT rc; GetClientRect(h, &rc);
            int w = rc.right, ht = rc.bottom;
            if (self && w > 0 && ht > 0) {   // drawn off-screen first, so it never flickers
                HDC mem = CreateCompatibleDC(dc);
                HBITMAP bmp = CreateCompatibleBitmap(dc, w, ht);
                HGDIOBJ oldBmp = SelectObject(mem, bmp);
                self->Paint(mem, w, ht);
                BitBlt(dc, 0, 0, w, ht, mem, 0, 0, SRCCOPY);
                SelectObject(mem, oldBmp);
                DeleteObject(bmp);
                DeleteDC(mem);
            }
            EndPaint(h, &ps);
            return 0;
        }
    }
    return DefWindowProcW(h, msg, wp, lp);
}
