#pragma once

#include "app/window.h"

#include <gtk/gtk.h>

namespace app {

class MainWindow {
public:
    MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context);

    gint width_ = 0;
    gint height_ = 0;
    gdouble mouse_x_ = 0;
    gdouble mouse_y_ = 0;

    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();
    bool isDarkMode();
    void setTitle(std::string_view title);
    Window* appWindow() const;
    GtkWidget* gtkWindow() const;
    GtkWidget* glArea() const {
        return gl_area;
    }

private:
    Window* app_window;
    GtkWidget* window;
    GtkWidget* gl_area;
};

}  // namespace app
