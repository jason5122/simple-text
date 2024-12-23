#include "main_window.h"
#include "unicode/unicode.h"
#include <cmath>

// Debug use; remove this.
#include <fmt/base.h>
#include <fmt/format.h>

namespace app {

namespace {

constexpr Key KeyFromKeyval(guint keyval);
constexpr ModifierKey ModifierFromState(guint state);
// TODO: Make this work with multiple modifiers.
constexpr const gchar* GtkAccelStringFromModifier(ModifierKey modifier);

// GtkWindow callbacks.
void destroy(GtkWidget* self, gpointer user_data);
// GtkGLArea callbacks.
GdkGLContext* create_context(GtkGLArea* self, gpointer user_data);
void realize(GtkGLArea* self, gpointer user_data);
gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data);
void resize(GtkGLArea* self, gint width, gint height, gpointer user_data);
gboolean scroll(GtkEventControllerScroll* self, gdouble dx, gdouble dy, gpointer user_data);
void pressed(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data);
void released(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data);
void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data);
void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app);
gboolean key_pressed(GtkEventControllerKey* self,
                     guint keyval,
                     guint keycode,
                     GdkModifierType state,
                     gpointer user_data);

}  // namespace

MainWindow::MainWindow(GtkApplication* gtk_app, Window* app_window, GdkGLContext* context)
    : app_window{app_window},
      window{gtk_application_window_new(gtk_app)},
      gl_area{gtk_gl_area_new()} {

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

        std::string quit_accel = fmt::format("{}q", GtkAccelStringFromModifier(kPrimaryModifier));
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
    g_signal_connect(gesture, "released", G_CALLBACK(released), app_window);

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
    fmt::println("~MainWindow");
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

void MainWindow::setTitle(std::string_view title) {
    gtk_window_set_title(GTK_WINDOW(window), title.data());
}

Window* MainWindow::appWindow() const {
    return app_window;
}

GtkWidget* MainWindow::gtkWindow() const {
    return window;
}

namespace {

void destroy(GtkWidget* self, gpointer user_data) {
    Window* app_window = static_cast<Window*>(user_data);
    app_window->onClose();
}

GdkGLContext* create_context(GtkGLArea* self, gpointer user_data) {
    GdkGLContext* context = static_cast<GdkGLContext*>(user_data);
    return g_object_ref(context);
}

void realize(GtkGLArea* self, gpointer user_data) {
    gtk_gl_area_make_current(self);
    if (gtk_gl_area_get_error(self) != nullptr) return;

    GdkGLAPI api = gtk_gl_area_get_api(self);
    if (api == GDK_GL_API_GL) {
        fmt::println("GDK_GL_API_GL");
    }
    if (api == GDK_GL_API_GLES) {
        fmt::println("GDK_GL_API_GLES");
    }

    // FIXME: Getting the size of a widget doesn't seem to be possible during realization.
    // This is fine for now since GTK sends a resize signal once the widget is created.
    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onOpenGLActivate({scaled_width, scaled_height});
}

void resize(GtkGLArea* self, gint width, gint height, gpointer user_data) {
    gtk_gl_area_make_current(self);

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onResize({width, height});
}

gboolean render(GtkGLArea* self, GdkGLContext* context, gpointer user_data) {
    gtk_gl_area_make_current(self);

    int scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    int scaled_width = gtk_widget_get_width(GTK_WIDGET(self)) * scale_factor;
    int scaled_height = gtk_widget_get_height(GTK_WIDGET(self)) * scale_factor;

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onDraw({scaled_width, scaled_height});

    // Draw commands are flushed after returning.
    return true;
}

gboolean scroll(GtkEventControllerScroll* self, gdouble dx, gdouble dy, gpointer user_data) {
    MainWindow* main_window = static_cast<MainWindow*>(user_data);

    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
    gtk_gl_area_make_current(GTK_GL_AREA(gl_area));

    GdkScrollUnit unit = gtk_event_controller_scroll_get_unit(self);
    if (unit == GDK_SCROLL_UNIT_WHEEL) {
        dx *= 32;
        dy *= 32;
    } else {
        // TODO: Figure out how to interpret scroll numbers (usually between 0.0-1.0).
        fmt::println("dy = {}", dy);
        // dx *= 32;
        // dy *= 32;
    }

    int mouse_x = std::round(main_window->mouse_x);
    int mouse_y = std::round(main_window->mouse_y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    Point mouse_pos = {mouse_x, mouse_y};
    mouse_pos *= scale_factor;

    Delta delta = {
        .dx = static_cast<int>(std::round(dx)),
        .dy = static_cast<int>(std::round(dy)),
    };

    Window* app_window = main_window->appWindow();
    app_window->onScroll(mouse_pos, delta);

    return true;
}

void pressed(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    GtkWidget* gl_area = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));

    int mouse_x = std::round(x);
    int mouse_y = std::round(y);

    int scale_factor = gtk_widget_get_scale_factor(gl_area);
    Point mouse_pos = {mouse_x, mouse_y};
    mouse_pos *= scale_factor;

    GdkModifierType event_state =
        gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));
    ModifierKey modifiers = ModifierFromState(event_state);
    ClickType click_type = ClickTypeFromCount(n_press);

    Window* app_window = static_cast<Window*>(user_data);
    app_window->onLeftMouseDown(mouse_pos, modifiers, click_type);
}

void released(GtkGestureClick* self, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    Window* app_window = static_cast<Window*>(user_data);
    app_window->onLeftMouseUp();
}

void motion(GtkEventControllerMotion* self, gdouble x, gdouble y, gpointer user_data) {
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
        Point mouse_pos = {mouse_x, mouse_y};
        mouse_pos *= scale_factor;

        ModifierKey modifiers = ModifierFromState(event_state);

        // TODO: Track click type. Consider having a member that tracks it from GtkGesture.
        Window* app_window = main_window->appWindow();
        app_window->onLeftMouseDrag(mouse_pos, modifiers, ClickType::kSingleClick);
    }
}

void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    fmt::println("quit callback");
    g_application_quit(G_APPLICATION(app));
}

gboolean key_pressed(GtkEventControllerKey* self,
                     guint keyval,
                     guint keycode,
                     GdkModifierType state,
                     gpointer user_data) {
    // TODO: Why does this occur?
    if (keyval == GDK_KEY_VoidSymbol) return false;

    Window* app_window = static_cast<Window*>(user_data);

    Key key = KeyFromKeyval(keyval);
    ModifierKey modifiers = ModifierFromState(state);
    bool handled = app_window->onKeyDown(key, modifiers);

    if (!handled) {
        guint32 codepoint = gdk_keyval_to_unicode(keyval);
        if (codepoint > 0) {
            char utf8[unicode::kMaxBytesInUTF8Sequence];
            size_t utf8_len = unicode::ToUTF8(codepoint, utf8);

            std::string str8(utf8, utf8_len);
            app_window->onInsertText(str8);

            fmt::println("str8 = {}, codepoint = {}", str8, codepoint);
        }
    }
    return false;  // TODO: See if we should return true/false here.
}

constexpr Key KeyFromKeyval(guint keyval) {
    // Convert to uppercase since GTK key events are case-sensitive.
    keyval = gdk_keyval_to_upper(keyval);

    constexpr struct {
        guint keyval;
        Key key;
    } kKeyMap[] = {
        {GDK_KEY_A, Key::kA},
        {GDK_KEY_B, Key::kB},
        {GDK_KEY_C, Key::kC},
        {GDK_KEY_D, Key::kD},
        {GDK_KEY_E, Key::kE},
        {GDK_KEY_F, Key::kF},
        {GDK_KEY_G, Key::kG},
        {GDK_KEY_H, Key::kH},
        {GDK_KEY_I, Key::kI},
        {GDK_KEY_J, Key::kJ},
        {GDK_KEY_K, Key::kK},
        {GDK_KEY_L, Key::kL},
        {GDK_KEY_M, Key::kM},
        {GDK_KEY_N, Key::kN},
        {GDK_KEY_O, Key::kO},
        {GDK_KEY_P, Key::kP},
        {GDK_KEY_Q, Key::kQ},
        {GDK_KEY_R, Key::kR},
        {GDK_KEY_S, Key::kS},
        {GDK_KEY_T, Key::kT},
        {GDK_KEY_U, Key::kU},
        {GDK_KEY_V, Key::kV},
        {GDK_KEY_W, Key::kW},
        {GDK_KEY_X, Key::kX},
        {GDK_KEY_Y, Key::kY},
        {GDK_KEY_Z, Key::kZ},
        {GDK_KEY_0, Key::k0},
        {GDK_KEY_1, Key::k1},
        {GDK_KEY_2, Key::k2},
        {GDK_KEY_3, Key::k3},
        {GDK_KEY_4, Key::k4},
        {GDK_KEY_5, Key::k5},
        {GDK_KEY_6, Key::k6},
        {GDK_KEY_7, Key::k7},
        {GDK_KEY_8, Key::k8},
        {GDK_KEY_9, Key::k9},
        {GDK_KEY_Return, Key::kEnter},
        {GDK_KEY_BackSpace, Key::kBackspace},
        {GDK_KEY_Tab, Key::kTab},
        {GDK_KEY_Left, Key::kLeftArrow},
        {GDK_KEY_Right, Key::kRightArrow},
        {GDK_KEY_Down, Key::kDownArrow},
        {GDK_KEY_Up, Key::kUpArrow},
    };
    for (size_t i = 0; i < std::size(kKeyMap); ++i) {
        if (kKeyMap[i].keyval == keyval) {
            return kKeyMap[i].key;
        }
    }
    return Key::kNone;
}

constexpr ModifierKey ModifierFromState(guint state) {
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
    if (state & GDK_SUPER_MASK || state & GDK_META_MASK) {
        modifiers |= ModifierKey::kSuper;
    }
    return modifiers;
}

// TODO: Make this work with multiple modifiers.
constexpr const gchar* GtkAccelStringFromModifier(ModifierKey modifier) {
    switch (modifier) {
    case ModifierKey::kShift:
        return "<Shift>";
    case ModifierKey::kControl:
        return "<Control>";
    case ModifierKey::kAlt:
        return "<Alt>";
    // TODO: Change this to <Super> after we finish testing. macOS's Command key maps to <Meta>.
    // https://docs.gtk.org/gdk4/flags.ModifierType.html
    case ModifierKey::kSuper:
        return "<Meta>";
    default:
        fmt::println("Error: Could not parse modifier.");
        std::abort();
    }
}

}  // namespace

}  // namespace app
