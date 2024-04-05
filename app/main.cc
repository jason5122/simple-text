#include "build/buildflag.h"

int SimpleTextMain(int argc, char* argv[]);

#if IS_WIN
#include "ui/app/win32/main_window.h"
#include <shellscalingapi.h>
#include <windows.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    // return SimpleTextMain(0, nullptr);

    MainWindow win;

    if (!win.create(L"Simple Text", WS_OVERLAPPEDWINDOW)) {
        return 0;
    }

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);

    // TODO: Don't hard code this!
    int scale_factor = 2;

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    };
    SetWindowPlacement(win.window(), &placement);

    // We need to pass `key` as a virtual key in order to combine it with FCONTROL.
    // https://stackoverflow.com/a/53657941
    ACCEL quit_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('q')),
        .cmd = ID_QUIT,
    };

    std::vector<ACCEL> accels = {quit_accel};
    HACCEL hAccel = CreateAcceleratorTable(&accels[0], accels.size());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
#else
int main(int argc, char* argv[]) {
    return SimpleTextMain(argc, argv);
}
#endif
