#include "app.h"
#include <gtk/gtk.h>

class App::impl {
public:
    GtkApplication* app;

    static void activate(GtkApplication* app);
    static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data);
};

gboolean App::impl::my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
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

void App::impl::activate(GtkApplication* app) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), app);

    gtk_window_maximize(GTK_WINDOW(window));
    // TODO: Set default window size without magic numbers.
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);

    gtk_widget_show_all(window);
}

App::App() : pimpl{new impl{}} {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    pimpl->app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(pimpl->app, "activate", G_CALLBACK(pimpl->activate), nullptr);
}

void App::run() {
    g_application_run(G_APPLICATION(pimpl->app), 0, NULL);
}

App::~App() {
    g_object_unref(pimpl->app);
}
