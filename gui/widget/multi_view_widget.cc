#include "multi_view_widget.h"

namespace gui {

MultiViewWidget::MultiViewWidget() {
    addTextView(std::make_shared<TextViewWidget>());
}

void MultiViewWidget::addTextView(std::shared_ptr<TextViewWidget> text_view) {
    text_views.push_back(std::move(text_view));
}

void MultiViewWidget::setIndex(size_t index) {
    this->index = index;
}

void MultiViewWidget::setContents(const std::string& text) {
    text_views[index]->setContents(text);
}

void MultiViewWidget::draw() {
    text_views[index]->draw();
}

void MultiViewWidget::scroll(const Point& mouse_pos, const Point& delta) {
    text_views[index]->scroll(mouse_pos, delta);
}

void MultiViewWidget::leftMouseDown(const Point& mouse_pos) {
    text_views[index]->leftMouseDown(mouse_pos);
}

void MultiViewWidget::leftMouseDrag(const Point& mouse_pos) {
    text_views[index]->leftMouseDrag(mouse_pos);
}

void MultiViewWidget::layout() {
    for (auto& text_view : text_views) {
        text_view->setSize(size);
        text_view->setPosition(position);
    }
}

}
