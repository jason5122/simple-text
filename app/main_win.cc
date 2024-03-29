#include "ui/win32/main_window.h"
#include "ui/win32/resource.h"
#include <iostream>
#include <minwindef.h>
#include <vector>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    MainWindow win;

    if (!win.Create(L"Learn to Program Windows", WS_OVERLAPPEDWINDOW)) {
        return 0;
    }

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement = {0};
    placement.length = sizeof(WINDOWPLACEMENT);
    placement.showCmd = SW_SHOWMAXIMIZED;
    placement.rcNormalPosition = RECT{0, 0, 1000, 500};
    SetWindowPlacement(win.Window(), &placement);

    // We need to pass `key` as a virtual key in order to combine it with FCONTROL.
    // https://stackoverflow.com/a/53657941
    ACCEL accel = {0};
    accel.fVirt = FCONTROL | FVIRTKEY;
    accel.key = LOBYTE(VkKeyScan('q'));
    accel.cmd = ID_QUIT;

    std::vector<ACCEL> accels = {accel};
    HACCEL hAccel = CreateAcceleratorTable(&accels[0], accels.size());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(win.Window(), hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
