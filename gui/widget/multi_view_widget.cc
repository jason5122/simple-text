#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "base/numeric/wrap_arithmetic.h"
#include "multi_view_widget.h"

namespace {

constexpr auto operator*(const std::string_view& sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) {
        result += sv;
    }
    return result;
}

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
const std::string json_with_escape [[maybe_unused]] = R"({
  "x": "\n",
})";
const std::string long_json_line =
    R"(   "file_exclude_patterns": ["*.pyc", "*.pyo", "*.exe", "*.dll", "*.obj","*.o", "*.a", "*.lib", "*.so", "*.dylib", "*.ncb", "*.sdf", "*.suo", "*.pdb", "*.idb", ".DS_Store", ".directory", "desktop.ini", "*.class", "*.psd", "*.db", "*.sublime-workspace"],
)";
const std::string large_json = "{\n" + long_json_line * 50 + "}";
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
    // addTab(json);
    // addTab(flat_json);
    addTab(json_with_escape);
    // addTab(large_json);
}

TextViewWidget* MultiViewWidget::currentTextViewWidget() const {
    if (!text_views.empty()) {
        return text_views[index].get();
    } else {
        return nullptr;
    }
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

void MultiViewWidget::draw(const std::optional<Point>& mouse_pos) {
    if (!text_views.empty()) {
        text_views[index]->draw(mouse_pos);
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
