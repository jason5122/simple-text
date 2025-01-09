#include "gui/app/menu.h"

#include "gui/app/gtk/impl_gtk.h"

namespace gui {

Menu::Menu() : pimpl{new impl{}} {
    pimpl->menu = g_menu_new();  // TODO: Use smart pointer.
}

Menu::~Menu() {
    g_object_unref(pimpl->menu);  // TODO: Use smart pointer.
}

void Menu::addItem(std::string_view label) {
    GMenuItem* item = g_menu_item_new("Expelliarmus", "app.expelliarmus");
    g_menu_append_item(pimpl->menu, item);
}

std::optional<size_t> Menu::show(const Point& mouse_pos) const {
    GtkWidget* popover_menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(pimpl->menu));
    gtk_widget_set_visible(popover_menu, true);
    return std::nullopt;
}

}  // namespace gui
