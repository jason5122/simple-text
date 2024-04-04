#include "app.h"
#include <gtk/gtk.h>

class App::impl {
public:
    GtkApplication* app;

    static void activate(GtkApplication* gtk_app, gpointer p_app);
    static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data);
};

gboolean App::impl::my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
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

void App::impl::activate(GtkApplication* gtk_app, gpointer p_app) {
    App* app = static_cast<App*>(p_app);
    app->onActivate();
}

App::App() : pimpl{new impl{}} {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    pimpl->app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(pimpl->app, "activate", G_CALLBACK(pimpl->activate), this);
}

void App::run() {
    g_application_run(G_APPLICATION(pimpl->app), 0, NULL);
}

void App::createNewWindow() {
    GtkWidget* window = gtk_application_window_new(pimpl->app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(pimpl->my_keypress_function),
                     pimpl->app);

    gtk_window_maximize(GTK_WINDOW(window));
    // TODO: Set default window size without magic numbers.
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);

    gtk_widget_show_all(window);
}

App::~App() {
    g_object_unref(pimpl->app);
}
