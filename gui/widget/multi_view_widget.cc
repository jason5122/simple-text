#include "multi_view_widget.h"

namespace gui {

MultiViewWidget::MultiViewWidget() {
    addTab("");
}

void MultiViewWidget::setIndex(int index) {
    this->index = index;
}

static inline int PositiveModulo(int i, int n) {
    return (i % n + n) % n;
}

void MultiViewWidget::prevIndex() {
    index = PositiveModulo(index - 1, text_views.size());
}

void MultiViewWidget::nextIndex() {
    index = PositiveModulo(index + 1, text_views.size());
}

void MultiViewWidget::addTab(const std::string& text) {
    text_views.emplace_back(new TextViewWidget{text});
}

void MultiViewWidget::selectAll() {
    text_views[index]->selectAll();
}

void MultiViewWidget::move(MoveBy by, bool forward, bool extend) {
    text_views[index]->move(by, forward, extend);
}

void MultiViewWidget::moveTo(MoveTo to, bool extend) {
    text_views[index]->moveTo(to, extend);
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
