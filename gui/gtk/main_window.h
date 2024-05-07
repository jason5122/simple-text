#pragma once

#include "gui/app.h"
#include "util/not_copyable_or_movable.h"
#include <gtk/gtk.h>

class MainWindow {
public:
    App::Window* app_window;

    NOT_COPYABLE(MainWindow)
    NOT_MOVABLE(MainWindow)
    MainWindow(GtkApplication* gtk_app, App::Window* app_window);
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
