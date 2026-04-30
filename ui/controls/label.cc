#include "ui/controls/label.h"
#include <utility>

namespace ui {

Label::Label(std::string text) : text_(std::move(text)) {}

void Label::set_text(std::string_view text) {
    text_ = text;
    invalidate();
}

Size Label::preferred_size(Size available) const { return Size{260, 28}; }

void Label::paint(PaintContext& context) {
    context.fill_rect(bounds(), Color{0.90f, 0.92f, 0.95f, 1.0f});
    Rect marker = bounds();
    marker.width = text_.empty() ? 24.0f : 120.0f;
    marker.x += 8.0f;
    marker.y += 8.0f;
    marker.height = 12.0f;
    context.fill_rect(marker, Color{0.35f, 0.40f, 0.48f, 1.0f});
    View::paint(context);
}

}  // namespace ui
