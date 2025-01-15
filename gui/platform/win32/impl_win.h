#pragma once

#include "gui/platform/app.h"
#include "gui/platform/menu.h"
#include "gui/platform/win32/dummy_context.h"
#include "gui/platform/win32/win32_window.h"
#include "gui/platform/window_widget.h"

namespace gui {

class App::impl {
public:
    DummyContext dummy_context;
};

class WindowWidget::impl {
public:
    impl(WindowWidget& app_window, DummyContext& dummy_context)
        : win32_window(app_window, dummy_context) {}

    Win32Window win32_window;

    // TODO: Implement unique class name creation in a better way.
    int wid = 0;
};

class Menu::impl {
public:
};

}  // namespace gui
