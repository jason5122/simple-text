#pragma once

#include "gui/window.h"
#include "util/not_copyable_or_movable.h"
#include <gtk/gtk.h>

namespace gui {

class MainWindow {
public:
    Window* app_window;

    NOT_COPYABLE(MainWindow)
    NOT_MOVABLE(MainWindow)
    MainWindow(GtkApplication* gtk_app, Window* app_window);
    ~MainWindow();
    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();
    bool isDarkMode();

private:
    GtkWidget* window;
    GtkWidget* gl_area;
    GDBusProxy* dbus_settings_proxy;
};

}
