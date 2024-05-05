#pragma once

#include "gui/app.h"
#include <gtk/gtk.h>

class MainWindow {
public:
    App::Window* app_window;

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
