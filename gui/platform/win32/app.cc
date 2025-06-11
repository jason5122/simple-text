#include "gui/platform/app.h"
#include "gui/platform/win32/impl_win.h"
#include "gui/platform/win32/resources.h"
#include <memory>
#include <string>
#include <windows.h>

namespace gui {

App::App() : pimpl{new impl{}} {}

App::~App() {}

void App::run() {
    pimpl->dummy_context.initialize();

    this->on_launch();

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

void App::quit() { PostQuitMessage(0); }

// TODO: Implement this.
std::string App::get_clipboard_string() { return ""; }

// TODO: Implement this.
void App::set_clipboard_string(const std::string& str8) { ; }

}  // namespace gui
