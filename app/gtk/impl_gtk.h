#pragma once

#include "app/app.h"
#include "app/gtk/main_window.h"
#include "app/menu.h"
#include "app/window.h"
#include <gtk/gtk.h>

namespace app {

class App::impl {
public:
    GtkApplication* app;
    GdkGLContext* context;
};

class Window::impl {
public:
    impl(GtkApplication* app, Window* app_window, GdkGLContext* context)
        : main_window(app, app_window, context) {}

    MainWindow main_window;
};

class Menu::impl {
public:
    GMenu* menu;  // TODO: Use smart pointer.
};

}  // namespace app
