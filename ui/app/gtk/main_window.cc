#include "main_window.h"
#include <glad/glad.h>
#include <iostream>

static gboolean key_press_event(GtkWidget* self, GdkEventKey* event, gpointer user_data);
static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data);
static void realize(GtkWidget* self, gpointer user_data);
static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);
static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data);

MainWindow::MainWindow(GtkApplication* gtk_app, App::Window* app_window)
    : window{gtk_application_window_new(gtk_app)}, app_window{app_window} {
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_event), this);

    GtkWidget* gl_area = gtk_gl_area_new();
    // g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), context);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), this);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), this);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), this);
    gtk_container_add(GTK_CONTAINER(window), gl_area);

    // gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
}

void MainWindow::show() {
    gtk_widget_show_all(window);
}

void MainWindow::close() {
    gtk_window_close(GTK_WINDOW(window));
}

static app::Key GetKey(guint vk) {
    static const struct {
        guint fVK;
        app::Key fKey;
    } gPair[] = {
        {GDK_KEY_A, app::Key::kA},
        {GDK_KEY_B, app::Key::kB},
        {GDK_KEY_C, app::Key::kC},
        // TODO: Implement the rest.
        {GDK_KEY_N, app::Key::kN},
        {GDK_KEY_Q, app::Key::kQ},
        {GDK_KEY_W, app::Key::kW},
    };

    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }

    return app::Key::kNone;
}

static gboolean key_press_event(GtkWidget* self, GdkEventKey* event, gpointer user_data) {
    app::Key key = GetKey(gdk_keyval_to_upper(event->keyval));

    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (event->state & GDK_SHIFT_MASK) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (event->state & GDK_CONTROL_MASK) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (event->state & GDK_MOD1_MASK) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if (event->state & GDK_SUPER_MASK) {
        modifiers |= app::ModifierKey::kSuper;
    }

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onKeyDown(key, modifiers);

    return true;
}

static void realize(GtkWidget* self, gpointer user_data) {
    gtk_gl_area_make_current(GTK_GL_AREA(self));
    if (gtk_gl_area_get_error(GTK_GL_AREA(self)) != nullptr) return;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    int scale_factor = gtk_widget_get_scale_factor(self);
    int scaled_width = gtk_widget_get_allocated_width(self) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(self) * scale_factor;

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onDraw();

    // Draw commands are flushed after returning.
    return true;
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->app_window->onResize(width, height);
}
