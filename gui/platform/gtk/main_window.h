#pragma once

#include "gui/platform/window_widget.h"
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
    int scale_factor();
    bool is_dark_mode();
    void set_title(std::string_view title);
    WindowWidget* app_window() const;
    GtkWidget* gtk_window() const;
    GtkWidget* gl_area() const { return gl_area_; }

private:
    WindowWidget* app_window_;
    GtkWidget* window_;
    GtkWidget* gl_area_;
};

}  // namespace gui
