#include "ui/controls/column.h"
#include <algorithm>

namespace ui {

Column::Column(float padding, float spacing) : padding_(padding), spacing_(spacing) {}

Size Column::preferred_size(Size available) const {
    float width = 0;
    float height = padding_ * 2.0f;
    for (const auto& child : children()) {
        Size child_size = child->preferred_size(available);
        width = std::max(width, child_size.width);
        height += child_size.height + spacing_;
    }
    if (!children().empty()) {
        height -= spacing_;
    }
    return Size{width + padding_ * 2.0f, height};
}

void Column::layout(Rect bounds) {
    View::layout(bounds);

    float y = bounds.y + padding_;
    float child_width = std::max(0.0f, bounds.width - padding_ * 2.0f);
    for (auto& child : children()) {
        Size child_size = child->preferred_size(Size{child_width, bounds.height});
        child->layout(Rect{bounds.x + padding_, y, child_width, child_size.height});
        y += child_size.height + spacing_;
    }
}

void Column::paint(PaintContext& context) {
    context.fill_rect(bounds(), Color{0.96f, 0.97f, 0.98f, 1.0f});
    View::paint(context);
}

}  // namespace ui
