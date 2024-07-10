#include "scrollable_widget.h"

namespace gui {

ScrollableWidget::ScrollableWidget(const Size& size)
    : Widget{size}, prev_scroll{std::chrono::system_clock::now()} {}

void ScrollableWidget::scroll(const Point& mouse_pos, const Point& delta) {
    auto curr_scroll = std::chrono::system_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(curr_scroll - prev_scroll).count();

    // Update previous scroll time.
    prev_scroll = curr_scroll;

    int x = std::abs(delta.x);
    int y = std::abs(delta.y);

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
        scroll_offset.y += delta.y;
    } else if (axis == ScrollAxis::Horizontal) {
        scroll_offset.x += delta.x;
    } else {
        scroll_offset.x += delta.x;
        scroll_offset.y += delta.y;
    }
    scroll_offset.x = std::clamp(scroll_offset.x, 0, max_scroll_offset.x);
    scroll_offset.y = std::clamp(scroll_offset.y, 0, max_scroll_offset.y);
}

}
