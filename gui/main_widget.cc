#include "main_widget.h"

namespace gui {

MainWidget::MainWidget(std::shared_ptr<renderer::Renderer> renderer) : Widget{renderer} {}

void MainWidget::draw(int width, int height) {
    for (auto& child : children) {
        child->draw(width, height);
    }
}

void MainWidget::scroll(int dx, int dy) {
    for (auto& child : children) {
        child->scroll(dx, dy);
    }
}

void MainWidget::addChild(std::unique_ptr<Widget> widget) {
    children.push_back(std::move(widget));
}

}
