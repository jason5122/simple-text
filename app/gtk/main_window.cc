#include "gdk/gdkkeysyms.h"
#include "main_window.h"
#include <cmath>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

#include <format>
#include <iostream>

namespace app {

// GtkWindow callbacks.
static void destroy(GtkWidget* self, gpointer user_data);
// GtkGLArea callbacks.
static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data);
static void realize(GtkGLArea* self, gpointer user_data);
static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);
static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data);
static gboolean scroll(GtkEventControllerScroll* self, gdouble dx, gdouble dy, gpointer user_data);

// TODO: Define this below instead.
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    std::cerr << "quit callback\n";
    g_application_quit(G_APPLICATION(app));
}

MainWindow::MainWindow(GtkApplication* gtk_app, app::Window* app_window, GdkGLContext* context)
    : window{gtk_application_window_new(gtk_app)},
      gl_area{gtk_gl_area_new()},
      app_window{app_window} {
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    GtkWidget* gtk_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), gtk_box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_widget_set_hexpand(gl_area, true);
    gtk_widget_set_vexpand(gl_area, true);
    gtk_box_append(GTK_BOX(gtk_box), gl_area);

    // Add menu bar.
    {
        GMenu* menu_bar = g_menu_new();
        GMenu* file_menu = g_menu_new();
        GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

        g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
        g_menu_append_item(file_menu, quit_menu_item);
        gtk_application_set_menubar(gtk_app, G_MENU_MODEL(menu_bar));

        gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

        const GActionEntry entries[] = {{"quit", quit_callback}};
        g_action_map_add_action_entries(G_ACTION_MAP(gtk_app), entries, G_N_ELEMENTS(entries),
                                        gtk_app);

        const gchar* quit_accels[2] = {"<Ctrl>q", NULL};
        gtk_application_set_accels_for_action(gtk_app, "app.quit", quit_accels);
    }

    // GtkWindow callbacks.
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), app_window);
    // GtkGLArea callbacks.
    g_signal_connect(gl_area, "create-context", G_CALLBACK(create_context), context);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), app_window);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), app_window);
    g_signal_connect(gl_area, "resize", G_CALLBACK(resize), app_window);

    GtkEventControllerScrollFlags scroll_flags = GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES;
    GtkEventController* scroll_event_controller = gtk_event_controller_scroll_new(scroll_flags);
    gtk_widget_add_controller(gl_area, scroll_event_controller);
    g_signal_connect(scroll_event_controller, "scroll", G_CALLBACK(scroll), app_window);

    // gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
}

MainWindow::~MainWindow() {
    std::cerr << "~MainWindow\n";
    // TODO: See if we need `g_object_unref` for any other object.
    // g_object_unref(dbus_settings_proxy);
}

void MainWindow::show() {
    gtk_window_present(GTK_WINDOW(window));
}

void MainWindow::close() {
    gtk_window_close(GTK_WINDOW(window));
}

void MainWindow::redraw() {
    gtk_widget_queue_draw(window);
}

int MainWindow::width() {
    // return gtk_widget_get_allocated_width(gl_area);
    // return gtk_widget_get_width(gl_area);
    return -1;
}

int MainWindow::height() {
    // return gtk_widget_get_allocated_height(gl_area);
    // return gtk_widget_get_height(gl_area);
    return -1;
}

int MainWindow::scaleFactor() {
    return gtk_widget_get_scale_factor(window);
}

bool MainWindow::isDarkMode() {
    // GError* error = nullptr;
    // GVariant* variant =
    //     g_dbus_proxy_call_sync(dbus_settings_proxy, "Read",
    //                            g_variant_new("(ss)", "org.freedesktop.appearance",
    //                            "color-scheme"), G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);

    // variant = g_variant_get_child_value(variant, 0);
    // while (variant && g_variant_is_of_type(variant, G_VARIANT_TYPE_VARIANT)) {
    //     // Unbox the return value.
    //     variant = g_variant_get_variant(variant);
    // }

    // // 0: default
    // // 1: dark
    // // 2: light
    // return g_variant_get_uint32(variant) == 1;
    return false;
}

void MainWindow::setTitle(const std::string& title) {
    gtk_window_set_title(GTK_WINDOW(window), &title[0]);
}

static void destroy(GtkWidget* self, gpointer user_data) {
    app::Window* app_window = static_cast<app::Window*>(user_data);
    app_window->onClose();
}

static GdkGLContext* create_context(GtkGLArea* self, gpointer user_data) {
    GdkGLContext* context = static_cast<GdkGLContext*>(user_data);
    return g_object_ref(context);
}

static void realize(GtkGLArea* self, gpointer user_data) {
    gtk_gl_area_make_current(self);
    if (gtk_gl_area_get_error(self) != nullptr) return;

    GdkGLAPI api = gtk_gl_area_get_api(self);
    if (api == GDK_GL_API_GL) {
        std::cerr << "GDK_GL_API_GL\n";
    }
    if (api == GDK_GL_API_GLES) {
        std::cerr << "GDK_GL_API_GLES\n";
    }

    // FIXME: Getting the size of a widget doesn't seem to be possible during realization.
    // This is fine for now since GTK sends a resize signal once the widget is created.
    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    app::Window* app_window = static_cast<app::Window*>(user_data);
    app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    app::Window* app_window = static_cast<app::Window*>(user_data);
    app_window->onResize(width, height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    app::Window* app_window = static_cast<app::Window*>(user_data);
    app_window->onDraw(scaled_width, scaled_height);

    // Draw commands are flushed after returning.
    return true;
}

static gboolean scroll(GtkEventControllerScroll* self,
                       gdouble dx,
                       gdouble dy,
                       gpointer user_data) {
    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
    gtk_gl_area_make_current(GTK_GL_AREA(gl_area));

    GdkScrollUnit unit = gtk_event_controller_scroll_get_unit(self);
    if (GDK_SCROLL_UNIT_WHEEL) {
        dx *= 32;
        dy *= 32;
    } else {
        // TODO: Figure out how to interpret scroll numbers (usually between 0.0-1.0).
        std::cerr << std::format("dy = {}", dy) << '\n';
        dx *= 32;
        dy *= 32;
    }

    int delta_x = std::round(dx);
    int delta_y = std::round(dy);
    app::Window* app_window = static_cast<app::Window*>(user_data);
    app_window->onScroll(delta_x, delta_y);

    gtk_widget_queue_draw(gl_area);

    return true;
}

}
