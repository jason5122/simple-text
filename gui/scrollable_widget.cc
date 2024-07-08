#include "scrollable_widget.h"

namespace gui {

void ScrollableWidget::scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) {
    scroll_offset.x += delta.x;
    scroll_offset.y += delta.y;
    scroll_offset.x = std::clamp(scroll_offset.x, 0, max_scroll_offset.x);
    scroll_offset.y = std::clamp(scroll_offset.y, 0, max_scroll_offset.y);
}

}
