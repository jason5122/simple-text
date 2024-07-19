#pragma once

#include "gui/widget/multi_view_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/types.h"
#include "gui/widget/vertical_layout_widget.h"

namespace gui {

class EditorWidget : public VerticalLayoutWidget {
public:
    EditorWidget();

    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    size_t getCurrentIndex();
    void addTab(const std::string& tab_name, const std::string& text);
    void removeTab(size_t index);
    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void insertText(std::string_view text);
    void leftDelete();

private:
    static constexpr int kTabBarHeight = 29 * 2;

    std::shared_ptr<MultiViewWidget> multi_view;
    std::shared_ptr<TabBarWidget> tab_bar;
};

}