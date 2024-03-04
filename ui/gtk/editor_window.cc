#include "editor_window.h"

static void print_hello(GtkWidget* widget, gpointer data) {
    g_print("Hello World\n");
}

// static void activate(GtkApplication* app, gpointer user_data) {
//     GtkWidget* window;
//     GtkWidget* button;

//     window = gtk_application_window_new(app);
//     gtk_window_set_title(GTK_WINDOW(window), "Hello");
//     gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

//     button = gtk_button_new_with_label("Hello World");
//     g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
//     gtk_window_set_child(GTK_WINDOW(window), button);

//     gtk_window_present(GTK_WINDOW(window));
// }

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    gtk_widget_show_all(window);
}

EditorWindow::EditorWindow() {
    app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
}

int EditorWindow::run() {
    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    return status;
}

EditorWindow::~EditorWindow() {
    g_object_unref(app);
}
