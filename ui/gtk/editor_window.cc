#include "editor_window.h"

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    gtk_widget_show_all(window);
}

EditorWindow::EditorWindow() {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    app = gtk_application_new("org.gtk.example", flags);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
}

int EditorWindow::run() {
    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    return status;
}

EditorWindow::~EditorWindow() {
    g_object_unref(app);
}
