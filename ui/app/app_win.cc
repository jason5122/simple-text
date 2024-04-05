#include "ui/app/app.h"
#include "ui/app/win32/base_window.h"
#include <glad/glad.h>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#define ID_QUIT 0x70
#define ID_CLOSE_WINDOW 0x71

const WCHAR CLASS_NAME[] = L"Sample Window Class";
const WCHAR WINDOW_NAME[] = L"Simple Text";

// TODO: Don't hard code this!
int scale_factor = 2;

class App::impl {
public:
    // std::vector<HWND> hwnds;
};

App::App() : pimpl{new impl{}} {
    WNDCLASS wc{
        .lpfnWndProc = Win32Window::WindowProc,
        .hInstance = GetModuleHandle(nullptr),
        // TODO: Change this color based on the editor background color.
        .hbrBackground = CreateSolidBrush(RGB(255, 0, 0)),
        // TODO: Change this.
        .lpszClassName = CLASS_NAME,
    };
    RegisterClass(&wc);

    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}

void App::run() {
    this->onActivate();

    // We need to pass `key` as a virtual key in order to combine it with FCONTROL.
    // https://stackoverflow.com/a/53657941
    ACCEL quit_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('q')),
        .cmd = ID_QUIT,
    };
    ACCEL close_accel{
        .fVirt = FCONTROL | FVIRTKEY,
        .key = LOBYTE(VkKeyScan('w')),
        .cmd = ID_CLOSE_WINDOW,
    };

    std::vector<ACCEL> accels = {quit_accel, close_accel};
    HACCEL hAccel = CreateAcceleratorTable(&accels[0], accels.size());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void App::createNewWindow() {
    Win32Window win;
    win.Create(WINDOW_NAME, WS_OVERLAPPEDWINDOW);

    // Win32Window window;
    // HWND hwnd = CreateWindowEx(0, CLASS_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW,
    // CW_USEDEFAULT,
    //                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr,
    //                            nullptr, GetModuleHandle(nullptr), &window);

    // ShowWindow(win.Window(), nCmdShow);

    // https://stackoverflow.com/a/20624817
    // FIXME: This doesn't animate like ShowWindow().
    // TODO: Replace magic numbers with actual defaults and/or window size restoration.
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        .showCmd = SW_SHOWNORMAL,
        // .showCmd = SW_SHOWMAXIMIZED,
        .rcNormalPosition = RECT{0, 0, 1000 * scale_factor, 500 * scale_factor},
    };
    SetWindowPlacement(win.Window(), &placement);
}

App::~App() {}
