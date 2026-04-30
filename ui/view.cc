#include "ui/view.h"

namespace ui {

Size View::preferred_size(Size available) const { return available; }

void View::layout(Rect bounds) {
    bounds_ = bounds;
    for (auto& child : children_) {
        child->layout(bounds);
    }
}

void View::paint(PaintContext& context) {
    for (auto& child : children_) {
        child->paint(context);
    }
}

View* View::hit_test(Point point) {
    if (!contains(point)) return nullptr;

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (View* target = (*it)->hit_test(point)) {
            return target;
        }
    }

    return accepts_pointer() ? this : nullptr;
}

bool View::on_pointer_move(const platform::PointerInfo& pointer_info) { return false; }

bool View::on_pointer_down(const platform::PointerInfo& pointer_info) { return false; }

bool View::on_pointer_up(const platform::PointerInfo& pointer_info) { return false; }

void View::set_invalidation_callback(std::function<void()> invalidate) {
    invalidate_ = std::move(invalidate);
    for (auto& child : children_) {
        child->set_invalidation_callback(invalidate_);
    }
}

void View::invalidate() {
    if (invalidate_) {
        invalidate_();
    }
}

}  // namespace ui
