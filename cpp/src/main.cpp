// Project OptM - entry point: Win32 window + Direct3D 11 + Dear ImGui.
#include <d3d11.h>
#include <dwmapi.h>
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <wincodec.h>
#include <vector>

#include <algorithm>

#include "app.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "util.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {
ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swapChain = nullptr;
ID3D11RenderTargetView* g_rtv = nullptr;
UINT g_resizeW = 0, g_resizeH = 0;
bool g_occluded = false;
App* g_app = nullptr;
UINT g_taskbarCreated = 0;

void CreateRenderTarget() {
    ID3D11Texture2D* back = nullptr;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) { g_device->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }
}
void CleanupRenderTarget() { if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; } }

bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                                               &sd, &g_swapChain, &g_device, &got, &g_context);
    if (hr == DXGI_ERROR_UNSUPPORTED)   // no usable GPU driver: fall back to the software renderer
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION,
                                           &sd, &g_swapChain, &g_device, &got, &g_context);
    if (FAILED(hr)) return false;
    CreateRenderTarget();
    return true;
}

// Developer aid: --screenshot <file.png> saves the first frames and exits (for checking layouts)
bool SaveBackbufferPng(const std::wstring& path) {
    ID3D11Texture2D* back = nullptr;
    if (FAILED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&back)))) return false;
    D3D11_TEXTURE2D_DESC d; back->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING; d.BindFlags = 0; d.CPUAccessFlags = D3D11_CPU_ACCESS_READ; d.MiscFlags = 0;
    ID3D11Texture2D* staging = nullptr;
    bool ok = SUCCEEDED(g_device->CreateTexture2D(&d, nullptr, &staging));
    if (ok) g_context->CopyResource(staging, back);
    back->Release();
    D3D11_MAPPED_SUBRESOURCE m;
    if (!ok || FAILED(g_context->Map(staging, 0, D3D11_MAP_READ, 0, &m))) { if (staging) staging->Release(); return false; }
    std::vector<BYTE> px(d.Width * d.Height * 4);
    for (UINT y = 0; y < d.Height; y++) {
        const BYTE* src = (const BYTE*)m.pData + y * m.RowPitch;
        BYTE* dst = px.data() + y * d.Width * 4;
        for (UINT x = 0; x < d.Width; x++) { dst[x * 4] = src[x * 4 + 2]; dst[x * 4 + 1] = src[x * 4 + 1]; dst[x * 4 + 2] = src[x * 4]; dst[x * 4 + 3] = 255; }
    }
    g_context->Unmap(staging, 0);
    staging->Release();

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* f = nullptr;
    IWICStream* s = nullptr;
    IWICBitmapEncoder* e = nullptr;
    IWICBitmapFrameEncode* fr = nullptr;
    ok = SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&f))) &&
         SUCCEEDED(f->CreateStream(&s)) && SUCCEEDED(s->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) &&
         SUCCEEDED(f->CreateEncoder(GUID_ContainerFormatPng, nullptr, &e)) && SUCCEEDED(e->Initialize(s, WICBitmapEncoderNoCache)) &&
         SUCCEEDED(e->CreateNewFrame(&fr, nullptr)) && SUCCEEDED(fr->Initialize(nullptr)) && SUCCEEDED(fr->SetSize(d.Width, d.Height));
    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
    ok = ok && SUCCEEDED(fr->SetPixelFormat(&fmt)) && SUCCEEDED(fr->WritePixels(d.Height, d.Width * 4, (UINT)px.size(), px.data())) &&
         SUCCEEDED(fr->Commit()) && SUCCEEDED(e->Commit());
    if (fr) fr->Release();
    if (e) e->Release();
    if (s) s->Release();
    if (f) f->Release();
    return ok;
}

void CleanupDevice() {
    CleanupRenderTarget();
    if (g_swapChain) g_swapChain->Release();
    if (g_context) g_context->Release();
    if (g_device) g_device->Release();
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;
    if (msg == g_taskbarCreated && g_taskbarCreated && g_app) { g_app->OnTaskbarCreated(); return 0; }
    switch (msg) {
        case WM_SIZE:
            if (wp == SIZE_MINIMIZED) { if (g_app) g_app->OnMinimize(); }   // lives on in the tray
            else { g_resizeW = LOWORD(lp); g_resizeH = HIWORD(lp); }
            return 0;
        case WM_APP_TRAY:
            if (g_app) g_app->OnTray(lp);
            return 0;
        case WM_HOTKEY:
            if (wp == kHotkeyPanic && g_app) g_app->OnHotkey();
            if (wp == kHotkeyOverlay && g_app) g_app->ToggleOverlay();
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(wp) != WA_INACTIVE && g_app) g_app->OnActivate();
            break;
        case WM_QUERYENDSESSION:
            return TRUE;
        case WM_ENDSESSION:
            if (wp && g_app) g_app->Shutdown();   // signing out / shutting down: put everything back now
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mm = (MINMAXINFO*)lp;
            float s = g_app ? g_app->Scale() : GetDpiForWindow(hwnd) / 96.0f;
            RECT work; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
            mm->ptMinTrackSize.x = std::min((LONG)(980 * s), work.right - work.left);
            mm->ptMinTrackSize.y = std::min((LONG)(620 * s), work.bottom - work.top);
            return 0;
        }
        case WM_DPICHANGED: {   // moved to a monitor with different scaling
            if (g_app) g_app->OnDpiChanged(HIWORD(wp) / 96.0f);
            const RECT* r = (const RECT*)lp;
            SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_KEYMENU) return 0;   // no Alt menu beep
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    std::wstring screenshot;
    int shotDelay = 2500, exitAfter = 0;   // exitAfter 0 = right after the screenshot
    bool tray = wcsstr(GetCommandLineW(), L"--tray") != nullptr;   // Start with Windows: start hidden in the tray
    int argc = 0;
    if (LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        for (int i = 1; i + 1 < argc; i++) {
            if (wcscmp(argv[i], L"--screenshot") == 0) screenshot = argv[i + 1];
            if (wcscmp(argv[i], L"--shot-delay") == 0) shotDelay = _wtoi(argv[i + 1]);
            if (wcscmp(argv[i], L"--exit-after") == 0) exitAfter = _wtoi(argv[i + 1]);
            // same as the OPTM_DATA_DIR / OPTM_LOG_FILE variables (which don't survive an elevation prompt)
            if (wcscmp(argv[i], L"--data-dir") == 0) SetEnvironmentVariableW(L"OPTM_DATA_DIR", argv[i + 1]);
            if (wcscmp(argv[i], L"--log-file") == 0) SetEnvironmentVariableW(L"OPTM_LOG_FILE", argv[i + 1]);
        }
        LocalFree(argv);
    }

    // Only one copy at a time - shared with the 1.x app, so the two never optimize at once.
    // After a self-update the old copy may still be closing, so wait for it.
    // (A test copy with its own data folder gets its own lock, so it can run next to the real one.)
    bool testCopy = !util::EnvVar(L"OPTM_DATA_DIR").empty();
    HANDLE mutex = CreateMutexW(nullptr, FALSE, testCopy ? L"Local\\ProjectOptMTestInstance" : L"Local\\ProjectOptMSingleInstance");
    DWORD waitMs = util::EnvVar(L"OPTM_RESTART").empty() ? 0 : 15000;
    SetEnvironmentVariableW(L"OPTM_RESTART", nullptr);
    DWORD got = mutex ? WaitForSingleObject(mutex, waitMs) : WAIT_FAILED;
    if (got != WAIT_OBJECT_0 && got != WAIT_ABANDONED) {
        if (screenshot.empty() && !tray)
            MessageBoxW(nullptr, L"Project OptM is already running. Look for its icon in the system tray.", L"Project OptM", MB_ICONINFORMATION);
        return 0;
    }
    // leftover from a self-update (the previous exe is renamed aside while it's running)
    std::wstring self = util::SelfPath();
    DeleteFileW((self + L".old").c_str());

    SetCurrentProcessExplicitAppUserModelID(L"ProjectOptM.Optimizer");   // same taskbar ID as 1.x, so pins keep working
    // 1.x ran inside powershell.exe, so Windows kept PowerShell's jump list ("Run ISE as Administrator"...)
    // under our ID - clear it so right-clicking the taskbar button shows only Project OptM
    if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        ICustomDestinationList* jump = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&jump)))) {
            jump->DeleteList(L"ProjectOptM.Optimizer");
            jump->Release();
        }
        CoUninitialize();
    }
    ImGui_ImplWin32_EnableDpiAwareness();
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    HICON icon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0, 0, inst, icon, LoadCursorW(nullptr, IDC_ARROW),
                       nullptr, nullptr, L"ProjectOptM", icon };
    RegisterClassExW(&wc);

    RECT work; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int w = std::min((int)(1240 * scale), (int)(work.right - work.left - 40));
    int h = std::min((int)(820 * scale), (int)(work.bottom - work.top - 40));
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Project OptM", WS_OVERLAPPEDWINDOW,
                                work.left + (work.right - work.left - w) / 2, work.top + (work.bottom - work.top - h) / 2,
                                w, h, nullptr, nullptr, inst, nullptr);
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));

    if (!CreateDevice(hwnd)) {
        CleanupDevice();
        MessageBoxW(nullptr, L"Project OptM couldn't start Direct3D 11.", L"Project OptM", MB_ICONERROR);
        return 1;
    }
    ULONGLONG shotAt = GetTickCount64() + shotDelay;
    ULONGLONG exitAt = exitAfter ? GetTickCount64() + exitAfter : 0;
    bool shotTaken = false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;                     // no imgui.ini next to the exe
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    App app;
    g_app = &app;
    app.Init(hwnd, scale);

    // now that the interface size is known, size and center the window, then show it
    SIZE ws = app.WindowSize(work);
    SetWindowPos(hwnd, nullptr, work.left + (work.right - work.left - ws.cx) / 2, work.top + (work.bottom - work.top - ws.cy) / 2,
                 ws.cx, ws.cy, SWP_NOZORDER | SWP_NOACTIVATE);
    if (!tray || !screenshot.empty()) {
        ShowWindow(hwnd, screenshot.empty() ? SW_SHOWDEFAULT : SW_SHOWNOACTIVATE);
        UpdateWindow(hwnd);
    }

    // Render only when something happens (plus a slow 5 fps tick), so the app idles near 0% CPU.
    // The optimizer keeps ticking while the window is hidden in the tray.
    bool done = false;
    int burst = 3;
    while (!done) {
        bool hidden = IsIconic(hwnd) || !IsWindowVisible(hwnd);
        DWORD wait = hidden ? 250 : (burst > 0 || app.Busy()) ? 0 : 200;
        if (MsgWaitForMultipleObjects(0, nullptr, FALSE, wait, QS_ALLINPUT) == WAIT_OBJECT_0) burst = 4;
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;
        app.Update();
        if (!IsWindow(hwnd)) continue;
        if (!screenshot.empty() && GetTickCount64() >= std::max(shotAt + 5000, exitAt)) { DestroyWindow(hwnd); continue; }   // hidden: give up on the shot
        if (IsIconic(hwnd) || !IsWindowVisible(hwnd)) continue;
        if (g_occluded && g_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) continue;
        g_occluded = false;
        if (g_resizeW && g_resizeH) {
            CleanupRenderTarget();
            g_swapChain->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeW = g_resizeH = 0;
            CreateRenderTarget();
            burst = std::max(burst, 2);
        }

        if (app.NeedsRescale()) {   // interface size changed: fonts are rebuilt between frames
            ImGui_ImplDX11_InvalidateDeviceObjects();
            app.Rescale();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.Render();
        ImGui::Render();
        const float clear[4] = { 0, 0, 0, 1 };
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (!screenshot.empty() && !shotTaken && GetTickCount64() >= shotAt) { SaveBackbufferPng(screenshot); shotTaken = true; }
        if (shotTaken && GetTickCount64() >= exitAt) DestroyWindow(hwnd);
        HRESULT hr = g_swapChain->Present(1, 0);
        g_occluded = hr == DXGI_STATUS_OCCLUDED;
        if (burst > 0) burst--;
    }

    app.Shutdown();   // every priority, service and power plan back to how it was
    bool restart = app.WantsRestart(), restartHidden = app.RestartHidden();
    g_app = nullptr;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    if (IsWindow(hwnd)) DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, inst);

    if (restart) {   // self-update swapped the exe: start the new version
        ReleaseMutex(mutex);
        SetEnvironmentVariableW(L"OPTM_RESTART", L"1");
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        std::wstring cmd = L"\"" + self + L"\"" + (restartHidden ? L" --tray" : L"");
        if (CreateProcessW(self.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
    return 0;
}
