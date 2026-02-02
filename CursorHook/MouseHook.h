#pragma once

#include <windows.h>

#ifdef CURSORHOOK_EXPORTS
#define MOUSEHOOK_CLASS __declspec(dllexport)
#else
#define MOUSEHOOK_CLASS __declspec(dllimport)
#endif

class MOUSEHOOK_CLASS MouseHook {
public:
    typedef void (*MouseMoveCallback)(POINT pos, LPARAM lParam);

    MouseHook();
    ~MouseHook();

    bool Install(MouseMoveCallback callback);
    void Uninstall();
    bool IsInstalled() const;

private:
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK hook_;
    MouseMoveCallback callback_;
    bool installed_;
};

#ifdef CURSORHOOK_EXPORTS
#define CURSORHOOK_API __declspec(dllexport)
#else
#define CURSORHOOK_API __declspec(dllimport)
#endif

extern "C" {
    CURSORHOOK_API MouseHook* CreateMouseHook();
    CURSORHOOK_API void DestroyMouseHook(MouseHook* hook);
}