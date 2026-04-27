#include "ui/controls/button.h"
#include <utility>

namespace ui {

Button::Button(std::string text) : text_(std::move(text)) {}

void Button::set_text(std::string text) {
    text_ = std::move(text);
    invalidate();
}

void Button::on_click(std::function<void()> callback) { on_click_ = std::move(callback); }

Size Button::preferred_size(Size available) const {
    (void)available;
    return Size{180, 44};
}

void Button::paint(PaintContext& context) {
    Color fill = hovered_ ? Color{0.68f, 0.80f, 0.96f, 1.0f} : Color{0.78f, 0.86f, 0.98f, 1.0f};
    if (pressed_) {
        fill = Color{0.48f, 0.64f, 0.90f, 1.0f};
    }
    context.fill_rect(bounds(), fill);

    Rect marker = bounds();
    marker.x += 16.0f;
    marker.y += 15.0f;
    marker.width = text_.empty() ? 32.0f : 96.0f;
    marker.height = 14.0f;
    context.fill_rect(marker, Color{0.10f, 0.18f, 0.30f, 1.0f});
    View::paint(context);
}

void Button::on_pointer_enter() {
    hovered_ = true;
    invalidate();
}

void Button::on_pointer_exit() {
    hovered_ = false;
    pressed_ = false;
    invalidate();
}

bool Button::on_pointer_down(const platform::PointerInfo& pointer_info) {
    if (pointer_info.button != 0) return false;
    pressed_ = true;
    invalidate();
    return true;
}

bool Button::on_pointer_up(const platform::PointerInfo& pointer_info) {
    if (!pressed_) return false;
    pressed_ = false;
    if (contains(Point{pointer_info.x_px, pointer_info.y_px}) && on_click_) {
        on_click_();
    }
    invalidate();
    return true;
}

}  // namespace ui
