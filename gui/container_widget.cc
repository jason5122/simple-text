#include "container_widget.h"

namespace gui {

void ContainerWidget::scroll(const renderer::Point& delta) {
    if (main_widget) {
        main_widget->scroll(delta);
    }
    for (auto& child : children) {
        child->scroll(delta);
    }
}

void ContainerWidget::leftMouseDown(const renderer::Point& mouse) {
    if (main_widget) {
        main_widget->leftMouseDown(mouse);
    }
    for (auto& child : children) {
        child->leftMouseDown(mouse);
    }
}

void ContainerWidget::leftMouseDrag(const renderer::Point& mouse) {
    if (main_widget) {
        main_widget->leftMouseDrag(mouse);
    }
    for (auto& child : children) {
        child->leftMouseDrag(mouse);
    }
}

}
