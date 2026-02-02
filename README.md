# Windows Cursor Motion Blur

C++ implementation for adding motion blur effects to Windows cursors.

## Features

- Captures mouse movements via low-level hook


## Usage

Run `CursorMotionBlur.exe`. Motion blur will appear when you move the mouse.

To exit, kill `CursorMotionBlur.exe` in Task Manager.

## Building

Requires C++17 compatible compiler, CMake 3.15+, and Windows SDK.

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Notes

Simple implementation. Can be improved with better performance, configuration UI, or custom cursor support.