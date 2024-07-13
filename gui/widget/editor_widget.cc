#include "editor_widget.h"
#include "gui/widget/multi_view_widget.h"
#include "gui/widget/padding_widget.h"
#include "gui/widget/tab_bar_widget.h"

namespace gui {

EditorWidget::EditorWidget()
    : multi_view{new MultiViewWidget{}}, tab_bar{new TabBarWidget({0, kTabBarHeight})} {
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
}

void EditorWidget::nextIndex() {
    multi_view->nextIndex();
}

void EditorWidget::addTab(const std::string& text) {
    multi_view->addTab(text);
}

}
