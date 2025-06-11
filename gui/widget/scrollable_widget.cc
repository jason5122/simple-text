#include "gui/widget/scrollable_widget.h"
#include <algorithm>

// References:
// https://github.com/zed-industries/zed/blob/40ecc38dd25ffdec4deb6e27ee91b72e85a019eb/crates/editor/src/scroll.rs#L129

namespace gui {

ScrollableWidget::ScrollableWidget(const Size& size)
    : Widget{size}, prev_scroll{std::chrono::system_clock::now()} {}

void ScrollableWidget::perform_scroll(const Point& mouse_pos, const Delta& delta) {
    auto curr_scroll = std::chrono::system_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(curr_scroll - prev_scroll).count();

    // Update previous scroll time.
    prev_scroll = curr_scroll;

    int x = std::abs(delta.dx);
    int y = std::abs(delta.dy);

    if (duration > kScrollEventSeparation) {
        axis = x <= y ? ScrollAxis::Vertical : ScrollAxis::Horizontal;
    } else if (std::max(x, y) >= kUnlockLowerBound) {
        if (axis == ScrollAxis::Vertical) {
            if (x > y && x >= y * kUnlockPercent) {
                axis = ScrollAxis::None;
            }
        } else if (axis == ScrollAxis::Horizontal) {
            if (y > x && y >= x * kUnlockPercent) {
                axis = ScrollAxis::None;
            }
        }
    }

    if (axis == ScrollAxis::Vertical) {
        scroll_offset.y += delta.dy;
    } else if (axis == ScrollAxis::Horizontal) {
        scroll_offset.x += delta.dx;
    } else {
        scroll_offset.x += delta.dx;
        scroll_offset.y += delta.dy;
    }
    scroll_offset.x = std::clamp(scroll_offset.x, 0, max_scroll_offset.x);
    scroll_offset.y = std::clamp(scroll_offset.y, 0, max_scroll_offset.y);
}

}  // namespace gui
