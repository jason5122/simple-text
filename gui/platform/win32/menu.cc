#include "gui/platform/menu.h"

#include "gui/platform/win32/impl_win.h"

namespace gui {

Menu::Menu() : pimpl{new impl{}} {}

Menu::~Menu() {}

void Menu::addItem(std::string_view label) {}

std::optional<size_t> Menu::show(const Point& mouse_pos) const {
    return std::nullopt;
}

}  // namespace gui
