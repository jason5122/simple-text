#include "ui/app/app.h"
#include <glad/glad.h>
#include <gtk/gtk.h>
#include <iostream>

class App::impl {
public:
    GtkApplication* app;
};

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_w && event->state & GDK_CONTROL_MASK) {
        gtk_window_close(GTK_WINDOW(widget));
        return true;
    }
    if (event->keyval == GDK_KEY_q && event->state & GDK_CONTROL_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    return false;
}

static void activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onActivate();
}

static void realize(GtkWidget* widget, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != nullptr) return;

    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;

    app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    app_window->onDraw();

    // Draw commands are flushed after returning.
    return true;
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    gtk_gl_area_make_current(self);

    app_window->onResize(width, height);
}

static gboolean scroll_event(GtkWidget* widget, GdkEventScroll* event, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    double dx, dy;
    gdk_event_get_scroll_deltas((GdkEvent*)event, &dx, &dy);

    if (gdk_device_get_source(event->device) == GDK_SOURCE_MOUSE) {
        std::cerr << "mouse\n";
        dx *= 32;
        dy *= 32;
    }

    app_window->onScroll(dx, dy);

    gtk_widget_queue_draw(widget);

    return true;
}

static gboolean button_event(GtkWidget* widget, GdkEventButton* event, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    if (event->type == GDK_BUTTON_PRESS) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y);

        gtk_widget_queue_draw(widget);
    }
    return true;
}

static gboolean motion_event(GtkWidget* widget, GdkEventMotion* event, gpointer p_app_window) {
    AppWindow* app_window = static_cast<AppWindow*>(p_app_window);

    if (event->type == GDK_MOTION_NOTIFY) {
        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        float scaled_mouse_x = mouse_x * scale_factor;
        float scaled_mouse_y = mouse_y * scale_factor;

        app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y);

        gtk_widget_queue_draw(widget);
    }
    return true;
}

App::App() : pimpl{new impl{}} {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    pimpl->app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(pimpl->app, "activate", G_CALLBACK(activate), this);
}

void App::run() {
    g_application_run(G_APPLICATION(pimpl->app), 0, NULL);
}

void App::createNewWindow(AppWindow& app_window) {
    GtkWidget* window = gtk_application_window_new(pimpl->app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, false);
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), &app_window);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), &app_window);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), &app_window);

    gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), &app_window);

    gtk_widget_add_events(gl_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(gl_area), "button-press-event", G_CALLBACK(button_event),
                     &app_window);

    gtk_widget_add_events(gl_area, GDK_BUTTON1_MOTION_MASK);
    g_signal_connect(G_OBJECT(gl_area), "motion-notify-event", G_CALLBACK(motion_event),
                     &app_window);

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function),
                     pimpl->app);

    // gtk_window_maximize(GTK_WINDOW(window));
    // TODO: Set default window size without magic numbers.
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);

    gtk_widget_show_all(window);
}

App::~App() {
    g_object_unref(pimpl->app);
}
