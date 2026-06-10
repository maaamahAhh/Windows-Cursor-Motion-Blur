#include <windows.h>
#include "../CursorOverlay/OverlayWindow.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    OverlayWindow overlay;

    if (!overlay.Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize overlay window",
                    "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    overlay.Run();

    return 0;
}
