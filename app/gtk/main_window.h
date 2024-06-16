#pragma once

#include "gui/window.h"
#include "util/non_copyable.h"
#include <gtk/gtk.h>

namespace app {

class MainWindow {
public:
    MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context);
    ~MainWindow();
    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();
    bool isDarkMode();
    void setTitle(const std::string& title);

private:
    Window* app_window;
    GtkWidget* window;
    GtkWidget* gl_area;
    GDBusProxy* dbus_settings_proxy;
};

}
