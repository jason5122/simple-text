#pragma once

#include "gui/widget/widget.h"

namespace gui {

// TODO: Consider breaking off toggle-able state into a "ToggleButtonWidget" subclass.
class ButtonWidget : public Widget {
public:
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    bool getState() const;

private:
    bool state = false;
};

}  // namespace gui
