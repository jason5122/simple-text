#pragma once

#include "gui/window.h"
#include "util/non_copyable.h"
#include <gtk/gtk.h>

namespace gui {

class MainWindow : util::NonMovable {
public:
    Window* app_window;

    MainWindow(GtkApplication* gtk_app, Window* app_window);
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
    GtkWidget* window;
    GtkWidget* gl_area;
    GDBusProxy* dbus_settings_proxy;
};

}
