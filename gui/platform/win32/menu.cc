#include "app/menu.h"
#include "app/win32/impl_win.h"

namespace app {

Menu::Menu() : pimpl{new impl{}} {}

Menu::~Menu() {}

void Menu::addItem(std::string_view label) {}

std::optional<size_t> Menu::show(const Point& mouse_pos) const {
    return std::nullopt;
}

}  // namespace app
