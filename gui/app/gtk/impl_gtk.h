#pragma once

#include "gui/app/app.h"
#include "gui/app/gtk/main_window.h"
#include "gui/app/menu.h"
#include "gui/app/window_widget.h"

#include <gtk/gtk.h>

namespace gui {

class App::impl {
public:
    GtkApplication* app;
    GdkGLContext* context;
};

class WindowWidget::impl {
public:
    impl(GtkApplication* app, WindowWidget* app_window, GdkGLContext* context)
        : main_window(app, app_window, context) {}

    MainWindow main_window;
    bool has_tick_callback = false;
    guint tick_callback_id = 0;
    gint64 first_frame_time = 0;

    void setAutoRedraw(bool auto_redraw);
};

class Menu::impl {
public:
    GMenu* menu;  // TODO: Use smart pointer.
};

}  // namespace gui
