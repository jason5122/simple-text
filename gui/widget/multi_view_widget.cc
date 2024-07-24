#include "multi_view_widget.h"

namespace gui {

MultiViewWidget::MultiViewWidget() {
    // TODO: Don't create a tab by default. See if we can have zero tabs like Sublime Text.
    // addTab("Hi 💣🇺🇸 Hello world!");
    addTab("Hello world");
}

void MultiViewWidget::setIndex(int index) {
    if (index < text_views.size()) {
        this->index = index;
    }
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

size_t MultiViewWidget::getCurrentIndex() {
    return index;
}

void MultiViewWidget::addTab(const std::string& text) {
    text_views.emplace_back(new TextViewWidget{text});
}

void MultiViewWidget::removeTab(size_t index) {
    text_views.erase(text_views.begin() + index);
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

void MultiViewWidget::insertText(std::string_view text) {
    text_views[index]->insertText(text);
}

void MultiViewWidget::leftDelete() {
    text_views[index]->leftDelete();
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
