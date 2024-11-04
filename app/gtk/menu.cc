#include "app/gtk/impl_gtk.h"
#include "menu.h"

namespace app {

Menu::Menu() : pimpl{new impl{}} {}

Menu::~Menu() {}

void Menu::addItem(ItemType type) {}

void Menu::show(const Point& mouse_pos) const {}

}  // namespace app
