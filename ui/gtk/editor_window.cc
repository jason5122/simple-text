#include "editor_window.h"

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_q && event->state & GDK_META_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    return false;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    gtk_widget_show_all(window);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), app);
}

EditorWindow::EditorWindow() {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
}

int EditorWindow::run() {
    return g_application_run(G_APPLICATION(app), 0, NULL);
}

EditorWindow::~EditorWindow() {
    g_object_unref(app);
}
