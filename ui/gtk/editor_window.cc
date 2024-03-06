#include "editor_window.h"
#include <epoxy/gl.h>
#include <iostream>

// FIXME: Remove these global variables! Figure out how to use classes with GTK.
RectRenderer* rect_renderer;
ImageRenderer* image_renderer;

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_q && event->state & GDK_META_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    if (event->keyval == GDK_KEY_q && event->state & GDK_CONTROL_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    return false;
}

static gboolean render(GtkWidget* widget) {
    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;
    int scaled_editor_offset_x = 200 * scale_factor;
    int scaled_editor_offset_y = 30 * scale_factor;

    // inside this function it's safe to use GL; the given
    // `GdkGLContext` has been made current to the drawable
    // surface used by the `GtkGLArea` and the viewport has
    // already been set to be the size of the allocation
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer->resize(scaled_width, scaled_height);
    rect_renderer->draw(0, 0, 0, 0, 40, 80, 1000, scaled_editor_offset_x, scaled_editor_offset_y);

    image_renderer->resize(scaled_width, scaled_height);
    image_renderer->draw(0, 0);

    // we completed our drawing; the draw commands will be
    // flushed at the end of the signal emission chain, and
    // the buffers will be drawn on the window
    return true;
}

static void realize(GtkWidget* widget) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != nullptr) return;

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;

    rect_renderer = new RectRenderer();
    image_renderer = new ImageRenderer();
    rect_renderer->setup(scaled_width, scaled_height);
    image_renderer->setup(scaled_width, scaled_height);
}

static gboolean resize(GtkWidget* widget, GdkEvent* event, gpointer data) {
    std::cerr << "resized\n";
    return false;
}

static void activate(GtkApplication* app) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(window, GDK_CONFIGURE);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), app);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(resize), nullptr);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, false);
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), nullptr);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), nullptr);

    gtk_widget_show_all(window);
}

EditorWindow::EditorWindow() {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
}

int EditorWindow::run() {
    return g_application_run(G_APPLICATION(app), 0, NULL);
}

EditorWindow::~EditorWindow() {
    g_object_unref(app);
}
