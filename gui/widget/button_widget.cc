#include "button_widget.h"

#include <fmt/base.h>

namespace gui {

void ButtonWidget::leftMouseDown(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {
    state = !state;
}

bool ButtonWidget::getState() const {
    return state;
}

}  // namespace gui
