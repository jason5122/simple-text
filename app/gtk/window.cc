#include "app/gtk/impl_gtk.h"
#include "app/window.h"

#include <fmt/base.h>

namespace app {

Window::Window(App& app, int width, int height)
    : pimpl{new impl{app.pimpl->app, this, app.pimpl->context}} {}

Window::~Window() {}

void Window::show() {
    pimpl->main_window.show();
}

void Window::close() {
    pimpl->main_window.close();
}

void Window::redraw() {
    pimpl->main_window.redraw();
}

int Window::width() const {
    return pimpl->main_window.width();
}

int Window::height() const {
    return pimpl->main_window.height();
}

int Window::scale() const {
    return pimpl->main_window.scaleFactor();
}

bool Window::isDarkMode() const {
    // return pimpl->main_window.isDarkMode();
    return false;
}

void Window::setTitle(std::string_view title) {
    pimpl->main_window.setTitle(title);
}

// TODO: Implement this.
void Window::setFilePath(std::string_view path) {}

// TODO: Implement this.
std::optional<std::string> Window::openFilePicker() const {
    return {};
}

// void Window::createMenuDebug() const {
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

}  // namespace app
