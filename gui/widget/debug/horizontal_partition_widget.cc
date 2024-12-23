#include "horizontal_partition_widget.h"

namespace gui {

HorizontalPartitionWidget::HorizontalPartitionWidget(const app::Size& size) : LayoutWidget{size} {}

void HorizontalPartitionWidget::layout() {
    size_t child_count = children_start.size();
    int child_width = size.width / child_count;

    for (size_t i = 0; i < child_count; ++i) {
        auto& child = children_start[i];
        child->setPosition({static_cast<int>(child_width * i), position.y});
        child->setWidth(child_width);
        child->setHeight(size.height);

        // Recursively layout children.
        child->layout();
    }
}

}  // namespace gui
