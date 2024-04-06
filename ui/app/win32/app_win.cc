#include "ui/app/app.h"
#include "ui/app/win32/main_window.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <memory>
#include <vector>
#include <windows.h>

class App::impl {
public:
    std::vector<std::unique_ptr<MainWindow>> windows;
};

App::App() : pimpl{new impl{}} {
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

void App::createNewWindow(AppWindow& app_window) {
    std::unique_ptr<MainWindow> window = std::make_unique<MainWindow>(app_window);
    window->create(L"Simple Text", WS_OVERLAPPEDWINDOW);

    // FIXME: This doesn't animate like ShowWindow().
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, 1000 * 2, 500 * 2},
    };
    SetWindowPlacement(window->hwnd, &placement);

    pimpl->windows.push_back(std::move(window));
}

App::~App() {}
