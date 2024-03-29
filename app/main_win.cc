#include "ui/win32/editor_window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(0,                            // Optional window styles.
                               CLASS_NAME,                   // Window class
                               L"Learn to Program Windows",  // Window text
                               WS_OVERLAPPEDWINDOW,          // Window style

                               // Size and position
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                               NULL,       // Parent window
                               NULL,       // Menu
                               hInstance,  // Instance handle
                               NULL        // Additional application data
    );

    if (hwnd == NULL) {
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
    SetWindowPlacement(hwnd, &placement);

    // FIXME: This is visually glitchy.
    // ShowWindow(hwnd, SW_SHOWMAXIMIZED);

    // Run the message loop.

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
