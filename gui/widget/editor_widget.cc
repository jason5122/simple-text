#include "base/filesystem/file_reader.h"
#include "editor_widget.h"
#include "gui/widget/padding_widget.h"

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

EditorWidget::EditorWidget()
    : multi_view{new MultiViewWidget<TextViewWidget>{}}, tab_bar{new TabBarWidget{kTabBarHeight}} {

    // Light.
    // constexpr Rgba kTabBarColor{190, 190, 190, 255};
    // constexpr Rgba kTextViewColor{253, 253, 253, 255};
    // Dark.
    constexpr Rgba kTabBarColor{79, 86, 94, 255};
    constexpr Rgba kTextViewColor{48, 56, 65, 255};

    // Leave padding between window title bar and tab.
    std::shared_ptr<Widget> tab_bar_padding{new PaddingWidget({0, 3 * 2}, kTabBarColor)};
    std::shared_ptr<Widget> text_view_padding{new PaddingWidget({0, 2 * 2}, kTextViewColor)};

    // std::string text = lorem;
    // std::string text = hello_emoji;
    // std::string text = R"(👩‍👩‍👧‍👦)";
    // std::string text = R"(🇺🇸)";
    // std::string text = R"(==👩‍👩‍👧‍👦﷽)";
    // std::string text = R"(==💣🇺🇸)";
    // std::string text = R"(apples != oranges >= bananas)";
    // std::string text = R"(🥲 != 💣 >= 🙂)";
    // std::string text = R"(🙂🙂🙂hi)";
    // std::string text = json;
    // std::string text = flat_json;
    std::string text = json_with_escape;
    // std::string text = large_json;
    // multi_view->addTab(std::make_shared<TextViewWidget>(text));
    multi_view->addTab(std::make_shared<TextViewWidget>(""));

    addChildStart(std::move(tab_bar_padding));
    addChildStart(tab_bar);
    addChildStart(std::move(text_view_padding));
    setMainWidget(multi_view);
}

TextViewWidget* EditorWidget::currentWidget() const {
    return multi_view->currentWidget();
}

void EditorWidget::setIndex(size_t index) {
    multi_view->setIndex(index);
    tab_bar->setIndex(index);
}

void EditorWidget::prevIndex() {
    multi_view->prevIndex();
    tab_bar->prevIndex();
}

void EditorWidget::nextIndex() {
    multi_view->nextIndex();
    tab_bar->nextIndex();
}

void EditorWidget::lastIndex() {
    multi_view->lastIndex();
    tab_bar->lastIndex();
}

size_t EditorWidget::getCurrentIndex() {
    return multi_view->getCurrentIndex();
}

void EditorWidget::addTab(std::string_view tab_name, std::string_view text) {
    multi_view->addTab(std::make_shared<TextViewWidget>(text));
    tab_bar->addTab(tab_name);
    layout();
}

void EditorWidget::removeTab(size_t index) {
    multi_view->removeTab(index);
    tab_bar->removeTab(index);
    layout();
}

void EditorWidget::openFile(std::string_view path) {
    std::string contents = base::ReadFile(path);
    addTab(path, contents);
}

}
