#pragma once

#include <windows.h>

#include "gui/platform/win32/dummy_context.h"
#include "gui/platform/window_widget.h"

namespace gui {

class Win32Window {
public:
    HWND m_hwnd;

    Win32Window(WindowWidget& app_window, DummyContext& dummy_context)
        : app_window{app_window}, dummy_context{dummy_context} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();
    int width();
    int height();
    int scale();
    void setTitle(std::string_view title);

private:
    HDC m_hdc;
    WindowWidget& app_window;
    DummyContext& dummy_context;

    WCHAR high_surrogate = '\0';
    LONG last_click_time = 0;
    int click_count = 0;
    bool tracking_mouse = false;
};

}  // namespace gui
