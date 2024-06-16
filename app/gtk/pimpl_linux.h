#pragma once

#include "gui/app.h"
#include "gui/gtk/main_window.h"
#include "gui/window.h"
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

}
