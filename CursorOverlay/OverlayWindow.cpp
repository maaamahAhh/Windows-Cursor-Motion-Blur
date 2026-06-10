#include "OverlayWindow.h"
#include <cstring>

static OverlayWindow* g_windowInstance = nullptr;

constexpr int BYTES_PER_PIXEL = 4;
constexpr BYTE ALPHA_MAX = 255;

OverlayWindow::OverlayWindow()
    : hwnd_(nullptr)
    , hInstance_(GetModuleHandle(nullptr))
    , hdc_(nullptr)
    , hdcMem_(nullptr)
    , hbmMem_(nullptr)
    , hbmOld_(nullptr)
    , pvBits_(nullptr)
    , screenWidth_(0)
    , screenHeight_(0)
    , cursorDc_(nullptr)
    , cursorDib_(nullptr)
    , cursorDibOld_(nullptr)
    , cursorWidth_(0)
    , cursorHeight_(0)
    , cursorHotspot_{0, 0}
    , mouseHook_(nullptr)
    , running_(false)
{
}

OverlayWindow::~OverlayWindow() {
    Close();
}

bool OverlayWindow::Initialize() {
    g_windowInstance = this;

    screenWidth_ = GetSystemMetrics(SM_CXSCREEN);
    screenHeight_ = GetSystemMetrics(SM_CYSCREEN);

    if (!RegisterWindowClass()) return false;
    if (!CreateWindowHandle()) return false;

    motionBlur_.initialize();

    if (!LoadCursorImage()) return false;

    mouseHook_ = CreateMouseHook();
    if (!mouseHook_) return false;

    if (!mouseHook_->Install([](POINT pos, LPARAM) {
        if (g_windowInstance) {
            g_windowInstance->OnMouseMove(pos, 0);
        }
    })) {
        DestroyMouseHook(mouseHook_);
        mouseHook_ = nullptr;
        return false;
    }

    if (!createFramebuffer()) return false;

    SetTimer(hwnd_, RENDER_TIMER_ID, RENDER_INTERVAL_MS, nullptr);
    return true;
}

void OverlayWindow::Run() {
    running_ = true;
    MSG msg;
    while (running_ && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void OverlayWindow::Close() {
    running_ = false;

    if (hwnd_) KillTimer(hwnd_, RENDER_TIMER_ID);

    if (mouseHook_) {
        mouseHook_->Uninstall();
        DestroyMouseHook(mouseHook_);
        mouseHook_ = nullptr;
    }

    ReleaseCursorResources();
    releaseFramebuffer();

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    motionBlur_.clear();
}

HWND OverlayWindow::GetHandle() const {
    return hwnd_;
}

bool OverlayWindow::RegisterWindowClass() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"CursorMotionBlurOverlay";
    return RegisterClassEx(&wc) != 0;
}

bool OverlayWindow::CreateWindowHandle() {
    hwnd_ = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"CursorMotionBlurOverlay",
        L"Cursor Motion Blur",
        WS_POPUP,
        0, 0, screenWidth_, screenHeight_,
        nullptr, nullptr, hInstance_, nullptr
    );
    if (!hwnd_) return false;

    ShowWindow(hwnd_, SW_SHOW);
    ::UpdateWindow(hwnd_);
    return true;
}

LRESULT CALLBACK OverlayWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_TIMER:
            if (wParam == RENDER_TIMER_ID && g_windowInstance) {
                g_windowInstance->RenderMotionBlur();
            }
            return 0;
        case WM_PAINT:
        case WM_RENDERBLUR:
            if (g_windowInstance) {
                g_windowInstance->RenderMotionBlur();
            }
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void OverlayWindow::OnMouseMove(POINT pos, LPARAM) {
    motionBlur_.addCursorPosition(pos);
    PostMessage(hwnd_, WM_RENDERBLUR, 0, 0);
}

void OverlayWindow::RenderMotionBlur() {
    if (!pvBits_ || !cursorDc_) return;

    clearFramebuffer();

    const auto ghosts = motionBlur_.getGhostPositions();
    const auto& config = motionBlur_.getConfig();

    for (int i = 0; i < config.ghostCount; ++i) {
        BYTE alpha = static_cast<BYTE>(ghostOpacityFor(i, config) * ALPHA_MAX);
        DrawCursorCopy(ghosts[i], alpha);
    }

    presentFramebuffer();
}

void OverlayWindow::DrawCursorCopy(POINT pos, BYTE alpha) {
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
    AlphaBlend(hdcMem_,
               pos.x - cursorHotspot_.x, pos.y - cursorHotspot_.y,
               cursorWidth_, cursorHeight_,
               cursorDc_, 0, 0,
               cursorWidth_, cursorHeight_, blend);
}

bool OverlayWindow::LoadCursorImage() {
    HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!hCursor) return false;

    ICONINFO iconInfo = {};
    if (!GetIconInfo(hCursor, &iconInfo)) return false;

    cursorHotspot_.x = iconInfo.xHotspot;
    cursorHotspot_.y = iconInfo.yHotspot;

    BITMAP bm = {};
    GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask,
              sizeof(BITMAP), &bm);
    cursorWidth_ = bm.bmWidth > 0 ? bm.bmWidth : 32;
    cursorHeight_ = bm.bmHeight > 0 ? bm.bmHeight : 32;

    HDC screenDc = GetDC(nullptr);
    cursorDib_ = createCursorDib(screenDc);
    cursorDc_ = CreateCompatibleDC(screenDc);
    cursorDibOld_ = (HBITMAP)SelectObject(cursorDc_, cursorDib_);

    if (iconInfo.hbmColor) {
        DrawIconEx(cursorDc_, 0, 0, hCursor,
                   cursorWidth_, cursorHeight_, 0, nullptr, DI_NORMAL);
    }

    ReleaseDC(nullptr, screenDc);

    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    return true;
}

void OverlayWindow::ReleaseCursorResources() {
    if (cursorDibOld_ && cursorDc_) {
        SelectObject(cursorDc_, cursorDibOld_);
        cursorDibOld_ = nullptr;
    }
    if (cursorDib_) {
        DeleteObject(cursorDib_);
        cursorDib_ = nullptr;
    }
    if (cursorDc_) {
        DeleteDC(cursorDc_);
        cursorDc_ = nullptr;
    }
}

bool OverlayWindow::createFramebuffer() {
    hdc_ = GetDC(hwnd_);
    hdcMem_ = CreateCompatibleDC(hdc_);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screenWidth_;
    bmi.bmiHeader.biHeight = -screenHeight_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbmMem_ = CreateDIBSection(hdc_, &bmi, DIB_RGB_COLORS, &pvBits_, nullptr, 0);
    if (!hbmMem_) return false;

    hbmOld_ = (HBITMAP)SelectObject(hdcMem_, hbmMem_);
    clearFramebuffer();
    return true;
}

void OverlayWindow::releaseFramebuffer() {
    if (hbmOld_ && hdcMem_) {
        SelectObject(hdcMem_, hbmOld_);
    }
    if (hbmMem_) {
        DeleteObject(hbmMem_);
        hbmMem_ = nullptr;
    }
    if (hdcMem_) {
        DeleteDC(hdcMem_);
        hdcMem_ = nullptr;
    }
    if (hdc_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
}

void OverlayWindow::clearFramebuffer() {
    std::memset(pvBits_, 0,
        static_cast<size_t>(screenWidth_) * screenHeight_ * BYTES_PER_PIXEL);
}

void OverlayWindow::presentFramebuffer() {
    POINT sourceOrigin = {0, 0};
    SIZE windowSize = {screenWidth_, screenHeight_};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, ALPHA_MAX, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd_, nullptr, nullptr, &windowSize,
                        hdcMem_, &sourceOrigin, 0, &blend, ULW_ALPHA);
}

HBITMAP OverlayWindow::createCursorDib(HDC compatibleDc) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cursorWidth_;
    bmi.bmiHeader.biHeight = -cursorHeight_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(compatibleDc, &bmi, DIB_RGB_COLORS,
                            nullptr, nullptr, 0);
}

float OverlayWindow::ghostOpacityFor(int ghostIndex,
                                      const MotionBlurConfig& config) const {
    if (config.ghostCount <= 1) return config.baseOpacity;

    float normalizedPosition = static_cast<float>(ghostIndex)
        / static_cast<float>(config.ghostCount - 1);
    return config.baseOpacity * (0.2f + 0.8f * normalizedPosition);
}
