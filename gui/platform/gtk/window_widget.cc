#include "gui/platform/gtk/impl_gtk.h"
#include "gui/platform/window_widget.h"
#include <fmt/base.h>
#include <string>

namespace gui {

WindowWidget::WindowWidget(App& app, int width, int height)
    : pimpl{new impl{app.pimpl->app, this, app.pimpl->context}} {}

WindowWidget::~WindowWidget() {}

void WindowWidget::show() { pimpl->main_window.show(); }

void WindowWidget::close() { pimpl->main_window.close(); }

void WindowWidget::redraw() { pimpl->main_window.redraw(); }

int WindowWidget::scale() const { return pimpl->main_window.scale_factor(); }

bool WindowWidget::is_dark_mode() const { return pimpl->main_window.is_dark_mode(); }

void WindowWidget::set_title(std::string_view title) { pimpl->main_window.set_title(title); }

// TODO: Implement this.
void WindowWidget::set_file_path(std::string_view path) {}

// TODO: Implement this.
std::optional<std::string> WindowWidget::open_file_picker() const { return {}; }

void WindowWidget::set_cursor_style(CursorStyle style) {
    GdkCursor* cursor = nullptr;
    if (style == CursorStyle::kArrow) {
        cursor = gdk_cursor_new_from_name("default", nullptr);
    } else if (style == CursorStyle::kIBeam) {
        cursor = gdk_cursor_new_from_name("text", nullptr);
    } else if (style == CursorStyle::kResizeLeftRight) {
        cursor = gdk_cursor_new_from_name("ew-resize", nullptr);
    } else if (style == CursorStyle::kResizeUpDown) {
        cursor = gdk_cursor_new_from_name("ns-resize", nullptr);
    }

    if (cursor) {
        GtkWidget* gtk_window = pimpl->main_window.gtkWindow();
        gtk_widget_set_cursor(gtk_window, cursor);
        g_object_unref(cursor);
    }
}

namespace {
gboolean TickCallback(GtkWidget* widget, GdkFrameClock* frame_clock, gpointer user_data) {
    auto* pimpl = static_cast<WindowWidget::impl*>(user_data);
    WindowWidget* app_window = pimpl->main_window.app_window();

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    if (pimpl->first_frame_time == 0) {
        pimpl->first_frame_time = frame_time;
    }

    gint64 d = frame_time - pimpl->first_frame_time;
    gint64 ms = d / 1000;
    app_window->onFrame(ms);

    return G_SOURCE_CONTINUE;
}
}  // namespace

void WindowWidget::impl::set_auto_redraw(bool auto_redraw) {
    GtkWidget* gtk_window = main_window.gl_area();
    if (!auto_redraw && has_tick_callback) {
        gtk_widget_remove_tick_callback(gtk_window, tick_callback_id);
        has_tick_callback = false;
        tick_callback_id = 0;
    } else if (auto_redraw && !has_tick_callback) {
        guint id = gtk_widget_add_tick_callback(gtk_window, &TickCallback, this, nullptr);
        has_tick_callback = true;
        tick_callback_id = id;
    }
}

// TODO: Refactor this.
void WindowWidget::set_auto_redraw(bool auto_redraw) { pimpl->set_auto_redraw(auto_redraw); }

// TODO: Implement.
int WindowWidget::frames_per_second() const { return 60; }

// void WindowWidget::createMenuDebug() const {
//     GtkWidget* window = pimpl->main_window.gtkWindow();

//     GMenu* menu = g_menu_new();
//     GMenuItem* item = g_menu_item_new("Expelliarmus", "app.expelliarmus");
//     g_menu_append_item(menu, item);
//     g_menu_append(menu, "Incendio", "app.incendio");

//     GdkRectangle rect{
//         .x = 0,
//         .y = 0,
//         .width = 0,
//         .height = 0,
//     };

//     GtkWidget* popover_menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
//     gtk_popover_set_has_arrow(GTK_POPOVER(popover_menu), false);
//     gtk_popover_set_pointing_to(GTK_POPOVER(popover_menu), &rect);
//     gtk_widget_set_parent(popover_menu, window);
//     // gtk_widget_set_visible(popover_menu, true);
//     gtk_popover_popup(GTK_POPOVER(popover_menu));

//     fmt::println("Context menu closed.");
// }

}  // namespace gui
