#include "button_widget.h"

#include <fmt/base.h>

namespace gui {

void ButtonWidget::leftMouseDown(const app::Point& mouse_pos,
                                 app::ModifierKey modifiers,
                                 app::ClickType click_type) {
    state = !state;
}

bool ButtonWidget::getState() const {
    return state;
}

}  // namespace gui
