#include "ui/app/app.h"
#include "ui/app/win32/main_window.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <memory>
#include <vector>
#include <windows.h>

class App::impl {
public:
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

App::~App() {}

class App::Window::impl {
public:
    impl(App::Window& app_window) : main_window(app_window) {}

    MainWindow main_window;
};

App::Window::Window(App& app) : parent(app), pimpl{new impl{*this}} {}

void App::Window::createWithSize(int width, int height) {
    pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW);

    // FIXME: This doesn't animate like ShowWindow().
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, width * 2, height * 2},
    };
    SetWindowPlacement(pimpl->main_window.hwnd, &placement);
}

void App::Window::close() {
    pimpl->main_window.destroy();
}
