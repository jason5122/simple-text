#include "app/app.h"
#include "app/win32/impl_win.h"
#include "app/win32/resources.h"
#include <memory>
#include <windows.h>

namespace app {

App::App() : pimpl{new impl{}} {}

App::~App() {}

void App::run() {
    pimpl->dummy_context.initialize();

    this->onLaunch();

    // https://stackoverflow.com/a/73504417/14698275
    ACCEL s_accel[1] = {{FCONTROL | FVIRTKEY, TEXT('Q'), ID_FILE_EXIT}};
    HACCEL h_accel = CreateAcceleratorTable(s_accel, 1);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, h_accel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyAcceleratorTable(h_accel);
}

void App::quit() {
    PostQuitMessage(0);
}

// TODO: Implement this.
std::string App::getClipboardString() {
    return "";
}

// TODO: Implement this.
void App::setClipboardString(const std::string& str8) {}

// TODO: Implement this.
void App::setCursorStyle(CursorStyle style) {}

}  // namespace app
