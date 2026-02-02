#include <windows.h>
#include <cstdio>
#include "../CursorOverlay/OverlayWindow.h"

void SetupConsole() {
    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    printf("Cursor Motion Blur - Started\n");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    OverlayWindow overlay;
    
    if (!overlay.Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize overlay window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    overlay.Run();

    CoUninitialize();

    return 0;
}