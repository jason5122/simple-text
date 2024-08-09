#include "main_window.h"
#include "unicode/unicode.h"
#include <cmath>

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
static void pressed(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data);
static void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data);
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app);
static gboolean key_pressed(GtkEventControllerKey* self,
                            guint keyval,
                            guint keycode,
                            GdkModifierType state,
                            gpointer user_data);

static constexpr const gchar* ConvertModifier(ModifierKey modifier) {
    switch (modifier) {
    case ModifierKey::kShift:
        return "<Shift>";
    case ModifierKey::kControl:
        return "<Control>";
    case ModifierKey::kAlt:
        return "<Alt>";
    // TODO: Figure out why this is <Meta> and not <Super>. We use GDK_SUPER_MASK elsewhere.
    case ModifierKey::kSuper:
        return "<Meta>";
    default:
        std::cerr << "Error: Could not parse modifier.\n";
        std::abort();
    }
}

MainWindow::MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context)
    : window{gtk_application_window_new(gtk_app)},
      gl_area{gtk_gl_area_new()},
      app_window{app_window} {
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    GtkWidget* gtk_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), gtk_box);

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

        std::string quit_accel = std::format("{}q", ConvertModifier(kPrimaryModifier), "q");
        const gchar* quit_accels[2] = {quit_accel.data(), NULL};
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
    g_signal_connect(scroll_event_controller, "scroll", G_CALLBACK(scroll), this);

    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_widget_add_controller(gl_area, GTK_EVENT_CONTROLLER(gesture));
    g_signal_connect(gesture, "pressed", G_CALLBACK(pressed), app_window);

    GtkEventController* motion_event_controller = gtk_event_controller_motion_new();
    gtk_widget_add_controller(gl_area, motion_event_controller);
    g_signal_connect(motion_event_controller, "motion", G_CALLBACK(motion), this);

    GtkEventController* key_event_controller = gtk_event_controller_key_new();
    gtk_widget_add_controller(window, key_event_controller);
    g_signal_connect(key_event_controller, "key-pressed", G_CALLBACK(key_pressed), app_window);

    gtk_widget_set_cursor(gl_area, gdk_cursor_new_from_name("text", nullptr));

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
    gtk_widget_queue_draw(gl_area);
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

Window* MainWindow::appWindow() {
    return app_window;
}

static void destroy(GtkWidget* self, gpointer user_data) {
    Window* app_window = static_cast<Window*>(user_data);
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

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onOpenGLActivate(scaled_width, scaled_height);
}

static void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onResize(width, height);
}

static gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onDraw(scaled_width, scaled_height);

    // Draw commands are flushed after returning.
    return true;
}

static gboolean scroll(GtkEventControllerScroll* self,
                       gdouble dx,
                       gdouble dy,
                       gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);

    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
    gtk_gl_area_make_current(GTK_GL_AREA(gl_area));

    GdkScrollUnit unit = gtk_event_controller_scroll_get_unit(self);
    if (GDK_SCROLL_UNIT_WHEEL) {
        dx *= 32;
        dy *= 32;
    } else {
        // TODO: Figure out how to interpret scroll numbers (usually between 0.0-1.0).
        std::cerr << std::format("dy = {}", dy) << '\n';
        // dx *= 32;
        // dy *= 32;
    }

    int mouse_x = std::round(main_window->mouse_x);
    int mouse_y = std::round(main_window->mouse_y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    int scaled_mouse_x = mouse_x * scale_factor;
    int scaled_mouse_y = mouse_y * scale_factor;

    int delta_x = std::round(dx);
    int delta_y = std::round(dy);
    Window* app_window = main_window->appWindow();
    std::cerr << std::format("x = {}, y = {}\n", scaled_mouse_x, scaled_mouse_y);
    app_window->onScroll(scaled_mouse_x, scaled_mouse_y, delta_x, delta_y);

    return true;
}

static constexpr ModifierKey ConvertGdkModifiers(guint state) {
    ModifierKey modifiers = ModifierKey::kNone;
    if (state & GDK_SHIFT_MASK) {
        modifiers |= ModifierKey::kShift;
    }
    if (state & GDK_CONTROL_MASK) {
        modifiers |= ModifierKey::kControl;
    }
    if (state & GDK_ALT_MASK) {
        modifiers |= ModifierKey::kAlt;
    }
    if (state & GDK_SUPER_MASK) {
        modifiers |= ModifierKey::kSuper;
    }
    return modifiers;
}

static void pressed(
    GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

    int mouse_x = std::round(x);
    int mouse_y = std::round(y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    int scaled_mouse_x = mouse_x * scale_factor;
    int scaled_mouse_y = mouse_y * scale_factor;

    GdkModifierType event_state =
        gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));
    ModifierKey modifiers = ConvertGdkModifiers(event_state);

    ClickType click_type = ClickType::kSingleClick;
    if (n_press == 2) {
        click_type = ClickType::kDoubleClick;
    } else if (n_press >= 3) {
        click_type = ClickType::kTripleClick;
    }

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onLeftMouseDown(scaled_mouse_x, scaled_mouse_y, modifiers, click_type);
}

static void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);
    main_window->mouse_x = x;
    main_window->mouse_y = y;

    GdkModifierType event_state =
        gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));

    // Left click drag.
    if (event_state & GDK_BUTTON1_MASK) {
        GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

        int mouse_x = std::round(x);
        int mouse_y = std::round(y);

        int scale_factor = gtk_widget_get_scale_factor(gl_area);
        int scaled_mouse_x = mouse_x * scale_factor;
        int scaled_mouse_y = mouse_y * scale_factor;

        ModifierKey modifiers = ConvertGdkModifiers(event_state);

        Window* app_window = main_window->appWindow();
        app_window->onLeftMouseDrag(scaled_mouse_x, scaled_mouse_y, modifiers);
    }
}

static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    std::cerr << "quit callback\n";
    g_application_quit(G_APPLICATION(app));
}

static gboolean key_pressed(GtkEventControllerKey* self,
                            guint keyval,
                            guint keycode,
                            GdkModifierType state,
                            gpointer user_data) {
    guint32 codepoint = gdk_keyval_to_unicode(keyval);
    if (codepoint > 0) {
        char utf8[unicode::kMaxBytesInUTF8Sequence];
        size_t utf8_len = unicode::ToUTF8(codepoint, utf8);

        std::string str8(utf8, utf8_len);
        Window* app_window = static_cast<Window*>(user_data);
        app_window->onInsertText(str8);

        std::cerr << std::format("str8 = {}, codepoint = {}\n", str8, codepoint);
    }
    return false;
}

}
