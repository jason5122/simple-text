#include "custom_gl_window.h"
#include <EGL/egl.h>
#include <GL/gl.h>
#include <gdk/gdkwayland.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

static void realize_cb(GtkWidget* widget, gpointer user_data);
static gboolean draw_cb(GtkWidget* widget, cairo_t* cr, gpointer user_data);

CustomGLWindow::CustomGLWindow(GtkApplication* gtk_app)
    : window{gtk_application_window_new(gtk_app)} {
    gtk_widget_set_double_buffered(window, false);
    g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(realize_cb), this);
    g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(draw_cb), this);
}

void CustomGLWindow::show() {
    gtk_widget_show(window);
}

static void realize_cb(GtkWidget* widget, gpointer user_data) {
    CustomGLWindow* custom_gl_window = static_cast<CustomGLWindow*>(user_data);

    EGLConfig egl_config;
    EGLint n_config;
    EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};

    bool is_wayland = false;
    if (is_wayland) {
        custom_gl_window->egl_display =
            (EGLDisplay*)eglGetDisplay((EGLNativeDisplayType)gdk_wayland_display_get_wl_display(
                gtk_widget_get_display(widget)));
        eglInitialize(custom_gl_window->egl_display, nullptr, nullptr);
        eglChooseConfig(custom_gl_window->egl_display, attributes, &egl_config, 1, &n_config);
        eglBindAPI(EGL_OPENGL_API);
        custom_gl_window->egl_surface = (EGLSurface*)eglCreateWindowSurface(
            custom_gl_window->egl_display, egl_config,
            (EGLNativeWindowType)gdk_wayland_window_get_wl_surface(gtk_widget_get_window(widget)),
            nullptr);
        custom_gl_window->egl_context = (EGLContext*)eglCreateContext(
            custom_gl_window->egl_display, egl_config, EGL_NO_CONTEXT, nullptr);
    } else {
        custom_gl_window->egl_display = (EGLDisplay*)eglGetDisplay(
            (EGLNativeDisplayType)gdk_x11_display_get_xdisplay(gtk_widget_get_display(widget)));
        eglInitialize(custom_gl_window->egl_display, nullptr, nullptr);
        eglChooseConfig(custom_gl_window->egl_display, attributes, &egl_config, 1, &n_config);
        eglBindAPI(EGL_OPENGL_API);
        custom_gl_window->egl_surface = (EGLSurface*)eglCreateWindowSurface(
            custom_gl_window->egl_display, egl_config,
            gdk_x11_window_get_xid(gtk_widget_get_window(widget)), nullptr);
        custom_gl_window->egl_context = (EGLContext*)eglCreateContext(
            custom_gl_window->egl_display, egl_config, EGL_NO_CONTEXT, nullptr);
    }

    eglMakeCurrent(custom_gl_window->egl_display, custom_gl_window->egl_surface,
                   custom_gl_window->egl_surface, custom_gl_window->egl_context);

    glClearColor(0, 1, 0, 1);
}

static gboolean draw_cb(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    CustomGLWindow* custom_gl_window = static_cast<CustomGLWindow*>(user_data);

    eglMakeCurrent(custom_gl_window->egl_display, custom_gl_window->egl_surface,
                   custom_gl_window->egl_surface, custom_gl_window->egl_context);

    glViewport(0, 0, gtk_widget_get_allocated_width(widget),
               gtk_widget_get_allocated_height(widget));

    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(custom_gl_window->egl_display, custom_gl_window->egl_surface);
    return true;
}
