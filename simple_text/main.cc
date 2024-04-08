#include "build/buildflag.h"

int SimpleTextMain(int argc, char* argv[]);

#if IS_WIN
#include <shellscalingapi.h>
#include <windows.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    return SimpleTextMain(0, nullptr);
}
#else
int main(int argc, char* argv[]) {
    return SimpleTextMain(argc, argv);
}
#endif
