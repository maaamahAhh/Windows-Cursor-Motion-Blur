#pragma once

#include <windows.h>
#include "../Core/MotionBlur.h"
#include "../CursorHook/MouseHook.h"

constexpr UINT WM_RENDERBLUR = WM_USER + 100;
constexpr UINT_PTR RENDER_TIMER_ID = 1;
constexpr UINT RENDER_INTERVAL_MS = 16;

class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Initialize();
    void Run();
    void Close();
    HWND GetHandle() const;

private:
    bool RegisterWindowClass();
    bool CreateWindowHandle();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnMouseMove(POINT pos, LPARAM lParam);
    void OnDisplayChange();
    void RenderMotionBlur();
    void DrawCursorCopy(POINT pos, BYTE alpha);
    bool LoadCursorImage();
    void ReleaseCursorResources();
    bool CreateFramebuffer();
    void ReleaseFramebuffer();
    void ClearFramebuffer();
    void PresentFramebuffer();
    HBITMAP CreateCursorDib(HDC compatibleDc);
    static BITMAPINFO CreateBitmapInfo(int width, int height);

    HWND hwnd_;
    HINSTANCE hInstance_;

    // Framebuffer resources
    HDC hdc_;
    HDC hdcMem_;
    HBITMAP hbmMem_;
    HBITMAP hbmOld_;
    VOID* pvBits_;
    int screenWidth_;
    int screenHeight_;

    // Cursor image resources
    HDC cursorDc_;
    HBITMAP cursorDib_;
    HBITMAP cursorDibOld_;
    int cursorWidth_;
    int cursorHeight_;
    POINT cursorHotspot_;

    MotionBlur motionBlur_;
    MouseHook* mouseHook_;
    bool running_;
};
