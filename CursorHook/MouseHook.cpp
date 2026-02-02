#define CURSORHOOK_EXPORTS
#include "MouseHook.h"

static MouseHook* g_hookInstance = nullptr;

MouseHook::MouseHook()
    : hook_(nullptr)
    , callback_(nullptr)
    , installed_(false)
{
}

MouseHook::~MouseHook() {
    Uninstall();
}

bool MouseHook::Install(MouseMoveCallback callback) {
    if (installed_) {
        return false;
    }

    if (!callback) {
        return false;
    }

    callback_ = callback;
    g_hookInstance = this;

    hook_ = SetWindowsHookEx(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandle(L"MouseHook.dll"),
        0
    );

    if (!hook_) {
        g_hookInstance = nullptr;
        callback_ = nullptr;
        return false;
    }

    installed_ = true;
    return true;
}

void MouseHook::Uninstall() {
    if (!installed_) {
        return;
    }

    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }

    installed_ = false;
    callback_ = nullptr;
    g_hookInstance = nullptr;
}

bool MouseHook::IsInstalled() const {
    return installed_;
}

LRESULT CALLBACK MouseHook::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_MOUSEMOVE) {
            MSLLHOOKSTRUCT* pMouseStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            if (pMouseStruct && g_hookInstance && g_hookInstance->callback_) {
                POINT pos = pMouseStruct->pt;
                g_hookInstance->callback_(pos, lParam);
            }
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

extern "C" {
    CURSORHOOK_API MouseHook* CreateMouseHook() {
        return new MouseHook();
    }

    CURSORHOOK_API void DestroyMouseHook(MouseHook* hook) {
        delete hook;
    }
}