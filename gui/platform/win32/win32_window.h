#pragma once

#include "gui/platform/win32/dummy_context.h"
#include "gui/platform/window_widget.h"
#include <windows.h>

namespace gui {

class Win32Window {
public:
    Win32Window(WindowWidget& app_window, DummyContext& dummy_context)
        : app_window{app_window}, dummy_context{dummy_context} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid);
    LRESULT handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();
    int width();
    int height();
    int scale();
    void set_title(std::string_view title);
    HWND get_hwnd() const;
    void set_hwnd(HWND hwnd);

private:
    HWND m_hwnd;
    HDC m_hdc;
    WindowWidget& app_window;
    DummyContext& dummy_context;

    // For WM_CHAR events.
    WCHAR high_surrogate = '\0';

    // Double-/triple-click tracking.
    LONG last_click_time = 0;
    int last_mouse_x = 0;
    int last_mouse_y = 0;
    int click_count = 0;

    // Helper for detecting when the mouse leaves the window on WM_MOUSEMOVE/WM_MOUSELEAVE events.
    bool tracking_mouse = false;
};

}  // namespace gui
