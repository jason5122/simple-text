#pragma once

#include "app/app.h"
#include "app/menu.h"
#include "app/win32/dummy_context.h"
#include "app/win32/main_window.h"
#include "app/window.h"

namespace app {

class App::impl {
public:
    DummyContext dummy_context;
};

class Window::impl {
public:
    impl(Window& app_window, DummyContext& dummy_context)
        : main_window(app_window, dummy_context) {}

    MainWindow main_window;

    // TODO: Implement unique class name creation in a better way.
    int wid = 0;
};

class Menu::impl {
public:
};

}  // namespace app
