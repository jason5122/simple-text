#pragma once

#include "gui/app.h"
#include "gui/gtk/main_window.h"
#include "gui/window.h"
#include <gtk/gtk.h>

namespace gui {

class App::impl {
public:
    GtkApplication* app;
};

class Window::impl {
public:
    impl(GtkApplication* app, Window* app_window) : main_window(app, app_window) {}

    MainWindow main_window;
};

}
