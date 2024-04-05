#include "build/buildflag.h"

int SimpleTextMain(int argc, char* argv[]);

#if IS_WIN
#define WIN32_LEAN_AND_MEAN
// https://stackoverflow.com/a/13420838/14698275
#define NOMINMAX
#include <windows.h>

#include <shellscalingapi.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    return SimpleTextMain(0, nullptr);
}
#else
int main(int argc, char* argv[]) {
    return SimpleTextMain(argc, argv);
}
#endif
