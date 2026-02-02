#pragma once

#include <windows.h>
#include "../Core/MotionBlur.h"
#include "../CursorHook/MouseHook.h"

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
    void RenderMotionBlur();
    void DrawCursorCopy(POINT pos, BYTE alpha);
    void UpdateWindow();
    bool LoadCursorImage();

    HWND hwnd_;
    HINSTANCE hInstance_;
    HDC hdc_;
    HDC hdcMem_;
    HBITMAP hbmMem_;
    HBITMAP hbmOld_;
    VOID* pvBits_;
    int screenWidth_;
    int screenHeight_;
    MotionBlur motionBlur_;
    MouseHook* mouseHook_;
    bool running_;
    POINT cursorHotspot_;
};