#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "multi_view_widget.h"

namespace {

const std::string lorem [[maybe_unused]] =
    R"(Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.)";
const std::string hello_emoji [[maybe_unused]] = R"(Hi 💣🇺🇸 Hello world!
This is a new line.)";
const std::string json [[maybe_unused]] = R"({
  "x": 10,
})";
const std::string flat_json [[maybe_unused]] = R"({ "x": 10 })";

}

namespace gui {

MultiViewWidget::MultiViewWidget() {
    // TODO: Don't create a tab by default. See if we can have zero tabs like Sublime Text.
    // addTab(lorem);
    // addTab(hello_emoji);
    // addTab(R"(👩‍👩‍👧‍👦)");
    // addTab(R"(🇺🇸)");
    // addTab(R"(==👩‍👩‍👧‍👦﷽)");
    // addTab(R"(==💣🇺🇸)");
    // addTab(R"(apples != oranges >= bananas)");
    // addTab(R"(🥲 != 💣 >= 🙂)");
    // addTab(R"(🙂🙂🙂hi)");
    addTab(json);
    // addTab(flat_json);
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

void MultiViewWidget::lastIndex() {
    index = base::sub_sat(text_views.size(), 1_Z);
}

size_t MultiViewWidget::getCurrentIndex() {
    return index;
}

void MultiViewWidget::addTab(std::string_view text) {
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

std::string MultiViewWidget::getSelectionText() {
    if (!text_views.empty()) {
        return text_views[index]->getSelectionText();
    } else {
        return "";
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

void MultiViewWidget::leftMouseDown(const Point& mouse_pos,
                                    app::ModifierKey modifiers,
                                    app::ClickType click_type) {
    if (!text_views.empty()) {
        text_views[index]->leftMouseDown(mouse_pos, modifiers, click_type);
    }
}

void MultiViewWidget::leftMouseDrag(const Point& mouse_pos,
                                    app::ModifierKey modifiers,
                                    app::ClickType click_type) {
    if (!text_views.empty()) {
        text_views[index]->leftMouseDrag(mouse_pos, modifiers, click_type);
    }
}

void MultiViewWidget::layout() {
    for (auto& text_view : text_views) {
        text_view->setSize(size);
        text_view->setPosition(position);
    }
}

}
