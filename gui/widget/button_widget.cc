#include "button_widget.h"

#include <fmt/base.h>

namespace gui {

void ButtonWidget::left_mouse_down(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {
    state = !state;
}

bool ButtonWidget::get_state() const {
    return state;
}

}  // namespace gui
