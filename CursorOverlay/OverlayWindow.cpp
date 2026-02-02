#include "OverlayWindow.h"
#include <cmath>

static OverlayWindow* g_windowInstance = nullptr;

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
    , mouseHook_(nullptr)
    , running_(false)
    , cursorHotspot_{0, 0}
{
}

OverlayWindow::~OverlayWindow() {
    Close();
}

bool OverlayWindow::Initialize() {
    g_windowInstance = this;

    screenWidth_ = GetSystemMetrics(SM_CXSCREEN);
    screenHeight_ = GetSystemMetrics(SM_CYSCREEN);

    if (!RegisterWindowClass()) {
        return false;
    }

    if (!CreateWindowHandle()) {
        return false;
    }

    motionBlur_.Initialize();

    if (!LoadCursorImage()) {
        return false;
    }

    mouseHook_ = CreateMouseHook();
    if (!mouseHook_) {
        return false;
    }

    if (!mouseHook_->Install([](POINT pos, LPARAM lParam) {
        if (g_windowInstance) {
            g_windowInstance->OnMouseMove(pos, lParam);
        }
    })) {
        DestroyMouseHook(mouseHook_);
        mouseHook_ = nullptr;
        return false;
    }

    hdc_ = GetDC(hwnd_);
    hdcMem_ = CreateCompatibleDC(hdc_);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screenWidth_;
    bmi.bmiHeader.biHeight = -screenHeight_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    pvBits_ = nullptr;
    hbmMem_ = CreateDIBSection(hdc_, &bmi, DIB_RGB_COLORS, &pvBits_, nullptr, 0);
    if (!hbmMem_) {
        return false;
    }
    hbmOld_ = (HBITMAP)SelectObject(hdcMem_, hbmMem_);

    if (pvBits_) {
        memset(pvBits_, 0, screenWidth_ * screenHeight_ * 4);
    }

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

    if (mouseHook_) {
        mouseHook_->Uninstall();
        DestroyMouseHook(mouseHook_);
        mouseHook_ = nullptr;
    }

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

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    motionBlur_.Clear();
}

HWND OverlayWindow::GetHandle() const {
    return hwnd_;
}

bool OverlayWindow::RegisterWindowClass() {
    WNDCLASSEX wc = {0};
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
        0,
        0,
        screenWidth_,
        screenHeight_,
        nullptr,
        nullptr,
        hInstance_,
        nullptr
    );

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    ::UpdateWindow(hwnd_);

    return true;
}

LRESULT CALLBACK OverlayWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            if (g_windowInstance) {
                g_windowInstance->RenderMotionBlur();
            }
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void OverlayWindow::OnMouseMove(POINT pos, LPARAM lParam) {
    motionBlur_.AddPosition(pos);

    UpdateWindow();
}

void OverlayWindow::RenderMotionBlur() {
    if (!hdc_ || !hdcMem_ || !pvBits_) {
        return;
    }

    memset(pvBits_, 0, screenWidth_ * screenHeight_ * 4);

    auto positions = motionBlur_.GetCursorCopyPositions();

    for (size_t i = 0; i < positions.size(); ++i) {
        float normalizedPos = static_cast<float>(i) / static_cast<float>(positions.size() - 1);
        float alpha = motionBlur_.GetConfig().baseOpacity * (0.2f + 0.8f * normalizedPos);

        BYTE alphaByte = static_cast<BYTE>(alpha * 255);
        DrawCursorCopy(positions[i], alphaByte);
    }

    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {screenWidth_, screenHeight_};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    UpdateLayeredWindow(hwnd_, nullptr, nullptr, &sizeWnd, hdcMem_, &ptSrc, 0, &blend, ULW_ALPHA);
}

void OverlayWindow::DrawCursorCopy(POINT pos, BYTE alpha) {
    HBITMAP hbmCursor = motionBlur_.GetCursorBitmap();
    if (!hbmCursor) {
        return;
    }

    BITMAP bm = {0};
    GetObject(hbmCursor, sizeof(BITMAP), &bm);

    HDC hdcTemp = CreateCompatibleDC(hdcMem_);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcTemp, hbmCursor);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = -bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    VOID* pvBits = nullptr;
    HBITMAP hbmAlpha = CreateDIBSection(hdcMem_, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
    HDC hdcAlpha = CreateCompatibleDC(hdcMem_);
    HBITMAP hbmAlphaOld = (HBITMAP)SelectObject(hdcAlpha, hbmAlpha);

    BitBlt(hdcAlpha, 0, 0, bm.bmWidth, bm.bmHeight, hdcTemp, 0, 0, SRCCOPY);

    BYTE* pPixels = (BYTE*)pvBits;
    for (int y = 0; y < bm.bmHeight; ++y) {
        for (int x = 0; x < bm.bmWidth; ++x) {
            int idx = (y * bm.bmWidth + x) * 4;
            BYTE b = pPixels[idx];
            BYTE g = pPixels[idx + 1];
            BYTE r = pPixels[idx + 2];
            BYTE a = pPixels[idx + 3];

            if (r > 0 || g > 0 || b > 0) {
                pPixels[idx + 3] = alpha;
            }
        }
    }

    BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
    AlphaBlend(hdcMem_, pos.x - cursorHotspot_.x, pos.y - cursorHotspot_.y,
               bm.bmWidth, bm.bmHeight, hdcAlpha, 0, 0, bm.bmWidth, bm.bmHeight, blend);

    SelectObject(hdcAlpha, hbmAlphaOld);
    DeleteObject(hbmAlpha);
    DeleteDC(hdcAlpha);

    SelectObject(hdcTemp, hbmOld);
    DeleteDC(hdcTemp);
}

void OverlayWindow::UpdateWindow() {
    InvalidateRect(hwnd_, nullptr, TRUE);
}

bool OverlayWindow::LoadCursorImage() {
    HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!hCursor) {
        return false;
    }

    ICONINFO iconInfo = {0};
    if (!GetIconInfo(hCursor, &iconInfo)) {
        return false;
    }

    cursorHotspot_.x = iconInfo.xHotspot;
    cursorHotspot_.y = iconInfo.yHotspot;

    BITMAP bm = {0};
    GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, sizeof(BITMAP), &bm);

    HDC hdc = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdc);
    hbmMem_ = CreateCompatibleBitmap(hdc, 32, 32);

    SelectObject(hdcMem, hbmMem_);
    if (iconInfo.hbmColor) {
        DrawIconEx(hdcMem, 0, 0, hCursor, 32, 32, 0, nullptr, DI_NORMAL);
    }

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdc);

    motionBlur_.SetCursorBitmap(hbmMem_);

    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);

    return true;
}