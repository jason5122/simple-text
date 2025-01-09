#pragma once

#include "gui/app/window_widget.h"

#include <gtk/gtk.h>

namespace gui {

class MainWindow {
public:
    MainWindow(GtkApplication* gtk_app, WindowWidget* app_window, GdkGLContext* context);

    gdouble mouse_x_ = 0;
    gdouble mouse_y_ = 0;

    void show();
    void close();
    void redraw();
    int scaleFactor();
    bool isDarkMode();
    void setTitle(std::string_view title);
    WindowWidget* appWindow() const;
    GtkWidget* gtkWindow() const;
    GtkWidget* glArea() const {
        return gl_area;
    }

private:
    WindowWidget* app_window;
    GtkWidget* window;
    GtkWidget* gl_area;
};

}  // namespace gui
