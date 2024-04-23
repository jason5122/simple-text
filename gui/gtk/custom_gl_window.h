#pragma once

#include <EGL/egl.h>
// #include <GL/gl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

class CustomGLWindow {
public:
    CustomGLWindow(GtkApplication* gtk_app);
    void show();

    // private:
    GtkWidget* window;
    EGLDisplay* egl_display;
    EGLSurface* egl_surface;
    EGLContext* egl_context;
};
