#include "overlay.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <objidl.h>
namespace Gdiplus { using std::min; using std::max; }   // its headers want min/max, which NOMINMAX takes away
#include <gdiplus.h>

namespace G = Gdiplus;

namespace {
const wchar_t* kClass = L"ProjectOptMOverlay";
const G::Color kText(255, 234, 236, 242), kDim(255, 150, 155, 170), kLabelBright(255, 200, 205, 216);   // labels with no background behind them
const G::Color kGood(255, 61, 220, 132), kSpike(255, 245, 185, 66);

float Pad() { return 12; }
float FpsRow() { return 34; }
float StatRow() { return 19; }
float GraphH() { return 20; }
float HintRow() { return 18; }
float Width() { return 196; }

G::Color FromRef(COLORREF c, BYTE a = 255) { return G::Color(a, GetRValue(c), GetGValue(c), GetBValue(c)); }

// Text as an outlined path, so it stays readable on a see-through background. Returns its width.
// (Font families are made per call: kept in statics they'd outlive GdiplusShutdown.)
float Text(G::Graphics& g, const std::wstring& t, float x, float y, float px, bool bold, const G::Color& fill, bool outline) {
    G::FontFamily semibold(L"Segoe UI Semibold"), plain(L"Segoe UI");
    const G::FontFamily* fam = semibold.IsAvailable() ? &semibold : &plain;
    G::GraphicsPath path;
    path.AddString(t.c_str(), -1, fam, bold ? G::FontStyleBold : G::FontStyleRegular, px, G::PointF(0, 0), nullptr);
    G::RectF b;
    path.GetBounds(&b);
    G::Matrix m;
    m.Translate(x, y);
    path.Transform(&m);
    if (outline) {
        G::Pen pen(G::Color(210, 0, 0, 0), std::max(2.0f, px * 0.16f));
        pen.SetLineJoin(G::LineJoinRound);
        g.DrawPath(&pen, &path);
    }
    G::SolidBrush brush(fill);
    g.FillPath(&brush, &path);
    return b.X + b.Width;
}

void RoundRect(G::GraphicsPath& p, float x, float y, float w, float h, float r) {
    float d = r * 2;
    p.AddArc(x, y, d, d, 180, 90);
    p.AddArc(x + w - d, y, d, d, 270, 90);
    p.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    p.AddArc(x, y + h - d, d, d, 90, 90);
    p.CloseFigure();
}
}  // namespace

bool Overlay::Create(HINSTANCE inst) {
    G::GdiplusStartupInput in;
    if (G::GdiplusStartup(&gdiplus_, &in, nullptr) != G::Ok) return false;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = Proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_SIZEALL);
    wc.lpszClassName = kClass;
    RegisterClassExW(&wc);
    // topmost, see-through for the mouse, never takes focus, not in the taskbar or Alt+Tab
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            kClass, L"Project OptM overlay", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr, inst, nullptr);
    if (!hwnd_) return false;
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, (LONG_PTR)this);
    return true;
}

void Overlay::Destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    if (gdiplus_) { G::GdiplusShutdown(gdiplus_); gdiplus_ = 0; }
    visible_ = false;
}

SIZE Overlay::Measure() const {
    float h = Pad() + FpsRow();
    size_t rows = (c_.stats.size() + 1) / 2;
    h += rows * StatRow();
    if (c_.showGraph) h += 6 + GraphH();
    if (moving_) h += 4 + HintRow();
    h += Pad() - 2;
    return { (LONG)std::lround(Width() * s_), (LONG)std::lround(h * s_) };
}

void Overlay::Show(HWND anchor, double fx, double fy, float scale, const OverlayContent& content) {
    if (!hwnd_ || !anchor) return;
    c_ = content;
    s_ = scale;
    SIZE sz = Measure();
    int x = pos_.x, y = pos_.y;
    if (moving_ && placedOnce_) {   // while you drag it, it stays where you put it (wherever the window is now)
        RECT wr;
        if (GetWindowRect(hwnd_, &wr)) { x = wr.left; y = wr.top; }
    } else {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(MonitorFromWindow(anchor, MONITOR_DEFAULTTONEAREST), &mi);
        const RECT& r = mi.rcMonitor;   // the whole screen - games cover the taskbar
        int m = (int)std::lround(14 * s_);
        x = r.left + m + (int)std::lround(std::clamp(fx, 0.0, 1.0) * std::max(0L, r.right - r.left - sz.cx - 2 * m));
        y = r.top + m + (int)std::lround(std::clamp(fy, 0.0, 1.0) * std::max(0L, r.bottom - r.top - sz.cy - 2 * m));
        placedOnce_ = true;
    }
    Render(x, y);
    uint64_t now = GetTickCount64();
    if (now - lastTopmost_ > 1000) {   // games and other overlays go topmost too - stay above them
        lastTopmost_ = now;
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (!visible_) { ShowWindow(hwnd_, SW_SHOWNOACTIVATE); visible_ = true; }
}

void Overlay::Hide() {
    if (hwnd_ && visible_) ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
}

void Overlay::SetMoving(bool on) {
    if (!hwnd_ || on == moving_) return;
    moving_ = on;
    placedOnce_ = false;
    LONG_PTR ex = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    ex = on ? (ex & ~WS_EX_TRANSPARENT) : (ex | WS_EX_TRANSPARENT);   // clickable only while moving
    SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

bool Overlay::TakeMoved(double& fx, double& fy) {
    if (!moved_) return false;
    moved_ = false;
    fx = movedX_; fy = movedY_;
    return true;
}

bool Overlay::TakeDone() { bool d = done_; done_ = false; return d; }

// dropped after a drag: where is it, as 0..1 across its screen?
void Overlay::Dropped() {
    RECT w; GetWindowRect(hwnd_, &w);
    pos_ = { w.left, w.top };
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& r = mi.rcMonitor;
    int m = (int)std::lround(14 * s_);
    double spanX = std::max(1L, r.right - r.left - (w.right - w.left) - 2 * m);
    double spanY = std::max(1L, r.bottom - r.top - (w.bottom - w.top) - 2 * m);
    movedX_ = std::clamp((w.left - r.left - m) / spanX, 0.0, 1.0);
    movedY_ = std::clamp((w.top - r.top - m) / spanY, 0.0, 1.0);
    moved_ = true;
}

void Overlay::Render(int x, int y) {
    SIZE sz = Measure();
    int w = sz.cx, h = sz.cy;
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    BITMAPINFO bi = {};
    bi.bmiHeader = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) { DeleteDC(mem); ReleaseDC(nullptr, screen); return; }
    {
        G::Bitmap bmp(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)bits);
        G::Graphics g(&bmp);
        g.SetSmoothingMode(G::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(G::PixelOffsetModeHalf);
        g.Clear(G::Color(0, 0, 0, 0));
        float s = s_;
        int bgA = (int)std::lround(c_.opacity * 2.55);
        if (moving_) bgA = std::max(bgA, 110);   // something to grab even when it's see-through
        bool outline = bgA < 150;
        if (bgA > 0) {
            G::GraphicsPath p;
            RoundRect(p, 0.5f, 0.5f, w - 1.0f, h - 1.0f, 10 * s);
            G::SolidBrush bg(G::Color((BYTE)bgA, 12, 13, 17));
            g.FillPath(&bg, &p);
        }
        if (moving_) {
            G::GraphicsPath p;
            RoundRect(p, 1.5f, 1.5f, w - 3.0f, h - 3.0f, 10 * s);
            G::Pen pen(FromRef(c_.accent), 2 * s);
            pen.SetDashStyle(G::DashStyleDash);
            g.DrawPath(&pen, &p);
        }

        // FPS
        float px = Pad() * s, y0 = Pad() * s - 5 * s;
        wchar_t b[32];
        if (c_.hasFps) swprintf(b, 32, L"%d", (int)std::lround(c_.fps)); else wcscpy_s(b, L"--");
        float fw = Text(g, b, px, y0, 28 * s, true, FromRef(c_.accent), outline);
        Text(g, L"FPS", px + fw + 5 * s, y0 + 13 * s, 12 * s, false, outline ? kLabelBright : kDim, outline);

        // stats, two per row
        float ty = (Pad() + FpsRow()) * s;
        float colW = (Width() - 2 * Pad()) / 2 * s;
        for (size_t i = 0; i < c_.stats.size(); i++) {
            float cx = px + (i % 2) * colW, cy = ty + (i / 2) * StatRow() * s;
            float lw = Text(g, c_.stats[i].label, cx, cy, 11.5f * s, false, outline ? kLabelBright : kDim, outline);
            Text(g, c_.stats[i].value, cx + lw + 5 * s, cy, 11.5f * s, false, kText, outline);
        }
        ty += ((c_.stats.size() + 1) / 2) * StatRow() * s;

        // mini frametime graph: one bar per column, spikes in amber
        if (c_.showGraph) {
            float gx = px, gw = w - 2 * px, gy = ty + 6 * s, gh = GraphH() * s;
            G::SolidBrush track(G::Color((BYTE)std::max(bgA / 2, outline ? 70 : 0), 30, 33, 42));
            g.FillRectangle(&track, gx, gy, gw, gh);
            std::vector<double> sorted;
            for (double v : c_.graph) if (v > 0) sorted.push_back(v);
            if (!sorted.empty()) {
                std::sort(sorted.begin(), sorted.end());
                double median = sorted[sorted.size() / 2];
                double top = std::max(sorted.back() * 1.15, median * 2.0);
                G::SolidBrush good(kGood), spike(kSpike);
                int n = (int)c_.graph.size();
                float bw = gw / n;
                for (int i = 0; i < n; i++) {
                    double v = c_.graph[i];
                    if (v <= 0) continue;
                    float bh = std::max(1.0f, (float)(gh * std::min(1.0, v / top)));
                    g.FillRectangle(v > median * 1.8 ? &spike : &good, gx + i * bw, gy + gh - bh, std::max(1.0f, bw - 0.5f), bh);
                }
            }
            ty = gy + gh;
        }
        if (moving_) Text(g, L"Drag me - double-click when done", px, ty + 4 * s, 10.5f * s, false, kText, true);
    }
    HGDIOBJ old = SelectObject(mem, dib);
    POINT dst = { x, y }, src = { 0, 0 };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd_, screen, &dst, &sz, mem, &src, 0, &bf, ULW_ALPHA);
    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    pos_ = { x, y };
    size_ = sz;
}

LRESULT CALLBACK Overlay::Proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = (Overlay*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_NCHITTEST: return self && self->moving_ ? HTCAPTION : HTTRANSPARENT;   // clicks go to the game
        case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
        case WM_EXITSIZEMOVE: if (self) self->Dropped(); return 0;
        case WM_NCLBUTTONDBLCLK: if (self && self->moving_) { self->done_ = true; return 0; } break;
    }
    return DefWindowProcW(h, msg, wp, lp);
}
