#pragma once

#include "app/win32/dummy_context.h"
#include "app/window.h"
#include <windows.h>

namespace app {

class Win32Window {
public:
    HWND m_hwnd;

    Win32Window(Window& app_window, DummyContext& dummy_context)
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
    Window& app_window;
    DummyContext& dummy_context;

    WCHAR high_surrogate = '\0';
    LONG last_click_time = 0;
    int click_count = 0;
    bool tracking_mouse = false;
};

}  // namespace app
