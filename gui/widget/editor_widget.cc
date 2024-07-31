#include "editor_widget.h"
#include "gui/widget/multi_view_widget.h"
#include "gui/widget/padding_widget.h"
#include "gui/widget/tab_bar_widget.h"

namespace gui {

EditorWidget::EditorWidget()
    : multi_view{new MultiViewWidget{}}, tab_bar{new TabBarWidget{kTabBarHeight}} {
    // Leave padding between window title bar and tab.
    constexpr Rgba kTabBarColor{190, 190, 190, 255};
    constexpr Rgba kTextViewColor{253, 253, 253, 255};
    std::shared_ptr<Widget> tab_bar_padding{new PaddingWidget({0, 3 * 2}, kTabBarColor)};
    std::shared_ptr<Widget> text_view_padding{new PaddingWidget({0, 2 * 2}, kTextViewColor)};

    addChildStart(std::move(tab_bar_padding));
    addChildStart(tab_bar);
    addChildStart(std::move(text_view_padding));
    setMainWidget(multi_view);
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

size_t EditorWidget::getCurrentIndex() {
    return multi_view->getCurrentIndex();
}

void EditorWidget::addTab(const std::string& tab_name, const std::string& text) {
    multi_view->addTab(text);
    tab_bar->addTab(tab_name);
    layout();
}

void EditorWidget::removeTab(size_t index) {
    multi_view->removeTab(index);
    tab_bar->removeTab(index);
    layout();
}

void EditorWidget::selectAll() {
    multi_view->selectAll();
}

void EditorWidget::move(MoveBy by, bool forward, bool extend) {
    multi_view->move(by, forward, extend);
}

void EditorWidget::moveTo(MoveTo to, bool extend) {
    multi_view->moveTo(to, extend);
}

void EditorWidget::insertText(std::string_view text) {
    multi_view->insertText(text);
}

void EditorWidget::leftDelete() {
    multi_view->leftDelete();
}

}
