#include "ui/app/app.h"
#include "ui/app/win32/main_window.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <memory>
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
    this->onLaunch();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

App::~App() {}

class App::Window::impl {
public:
    impl(App::Window& app_window) : main_window(app_window) {}

    MainWindow main_window;
};

App::Window::Window(App& parent, int width, int height)
    : pimpl{new impl{*this}}, parent(parent), ram_waster(5000000, 1) {
    pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW);
}

void App::Window::show() {
    // TODO: Sync this with requested width/height.
    int width = 1200;
    int height = 600;

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

App::Window::~Window() {}
