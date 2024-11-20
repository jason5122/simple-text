#include "base/filesystem/file_reader.h"
#include "editor_widget.h"
#include "gui/widget/padding_widget.h"

namespace gui {

EditorWidget::EditorWidget(size_t main_font_id, int main_font_size, size_t ui_font_size)
    : main_font_id(main_font_id),
      main_font_size(main_font_size),
      multi_view{new MultiViewWidget<TextViewWidget>{}},
      tab_bar{new TabBarWidget(ui_font_size, kTabBarHeight)} {
    // Leave padding between window title bar and tab.
    std::shared_ptr<Widget> tab_bar_padding{new PaddingWidget({0, 3 * 2}, kTabBarColor)};
    std::shared_ptr<Widget> text_view_padding{new PaddingWidget({0, 2 * 2}, kTextViewColor)};

    multi_view->addTab(std::make_shared<TextViewWidget>("", main_font_id, main_font_size));

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
    multi_view->addTab(std::make_shared<TextViewWidget>(text, main_font_id, main_font_size));
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

}  // namespace gui
