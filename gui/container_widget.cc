#include "container_widget.h"

namespace gui {

void ContainerWidget::setMainWidget(std::unique_ptr<Widget> widget) {
    main_widget = std::move(widget);
    layout();
}

void ContainerWidget::addChildStart(std::unique_ptr<Widget> widget) {
    children_start.push_back(std::move(widget));
    layout();
}

void ContainerWidget::addChildEnd(std::unique_ptr<Widget> widget) {
    children_end.push_back(std::move(widget));
    layout();
}

void ContainerWidget::draw() {
    if (main_widget) {
        main_widget->draw();
    }
    for (auto& child : children_start) {
        child->draw();
    }
    for (auto& child : children_end) {
        child->draw();
    }
}

void ContainerWidget::scroll(const renderer::Point& delta) {
    if (main_widget) {
        main_widget->scroll(delta);
    }
    for (auto& child : children_start) {
        child->scroll(delta);
    }
    for (auto& child : children_end) {
        child->scroll(delta);
    }
}

void ContainerWidget::leftMouseDown(const renderer::Point& mouse) {
    if (main_widget) {
        main_widget->leftMouseDown(mouse);
    }
    for (auto& child : children_start) {
        child->leftMouseDown(mouse);
    }
    for (auto& child : children_end) {
        child->leftMouseDown(mouse);
    }
}

void ContainerWidget::leftMouseDrag(const renderer::Point& mouse) {
    if (main_widget) {
        main_widget->leftMouseDrag(mouse);
    }
    for (auto& child : children_start) {
        child->leftMouseDrag(mouse);
    }
    for (auto& child : children_end) {
        child->leftMouseDrag(mouse);
    }
}

void ContainerWidget::setPosition(const renderer::Point& position) {
    this->position = position;
    layout();
}

}
