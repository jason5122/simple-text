#include "base/numeric/wrap_arithmetic.h"
#include "multi_view_widget.h"

namespace gui {

MultiViewWidget::MultiViewWidget() {
    // TODO: Don't create a tab by default. See if we can have zero tabs like Sublime Text.
    addTab(R"(Hi 💣🇺🇸 Hello world!
This is a new line.)");
    // addTab(R"(👩‍👩‍👧‍👦)");
    // addTab(R"(🇺🇸)");
    // addTab(R"(==👩‍👩‍👧‍👦﷽)");
    // addTab(R"(==💣🇺🇸)");
    // addTab(R"(apples != oranges >= bananas)");
    // addTab(R"(🥲 != 💣 >= 🙂)");
    // addTab(R"(🙂🙂🙂hi)");
}

void MultiViewWidget::setIndex(size_t index) {
    if (index < text_views.size()) {
        this->index = index;
    }
}

void MultiViewWidget::prevIndex() {
    index = base::dec_wrap(index, text_views.size());
}

void MultiViewWidget::nextIndex() {
    index = base::inc_wrap(index, text_views.size());
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
    if (!text_views.empty()) {
        text_views[index]->selectAll();
    }
}

void MultiViewWidget::move(MoveBy by, bool forward, bool extend) {
    if (!text_views.empty()) {
        text_views[index]->move(by, forward, extend);
    }
}

void MultiViewWidget::moveTo(MoveTo to, bool extend) {
    if (!text_views.empty()) {
        text_views[index]->moveTo(to, extend);
    }
}

void MultiViewWidget::insertText(std::string_view text) {
    if (!text_views.empty()) {
        text_views[index]->insertText(text);
    }
}

void MultiViewWidget::leftDelete() {
    if (!text_views.empty()) {
        text_views[index]->leftDelete();
    }
}

void MultiViewWidget::draw() {
    if (!text_views.empty()) {
        text_views[index]->draw();
    }
}

void MultiViewWidget::scroll(const Point& mouse_pos, const Point& delta) {
    if (!text_views.empty()) {
        text_views[index]->scroll(mouse_pos, delta);
    }
}

void MultiViewWidget::leftMouseDown(const Point& mouse_pos) {
    if (!text_views.empty()) {
        text_views[index]->leftMouseDown(mouse_pos);
    }
}

void MultiViewWidget::leftMouseDrag(const Point& mouse_pos) {
    if (!text_views.empty()) {
        text_views[index]->leftMouseDrag(mouse_pos);
    }
}

void MultiViewWidget::layout() {
    for (auto& text_view : text_views) {
        text_view->setSize(size);
        text_view->setPosition(position);
    }
}

}
