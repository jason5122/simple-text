#pragma once

#include "app/window.h"
#include <gtk/gtk.h>

namespace app {

class MainWindow {
public:
    MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context);
    ~MainWindow();

    // We track mouse position in `GtkEventControllerMotion` for use in `GtkEventControllerScroll`.
    gdouble mouse_x = 0;
    gdouble mouse_y = 0;

    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();
    bool isDarkMode();
    void setTitle(const std::string& title);
    Window* appWindow() const;
    GtkWidget* gtkWindow() const;

private:
    Window* app_window;
    GtkWidget* window;
    GtkWidget* gl_area;
    // GDBusProxy* dbus_settings_proxy;
};

}  // namespace app
