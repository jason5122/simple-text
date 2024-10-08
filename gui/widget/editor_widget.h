#pragma once

#include "gui/widget/container/multi_view_widget.h"
#include "gui/widget/container/vertical_layout_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/text_view_widget.h"

namespace gui {

class EditorWidget : public VerticalLayoutWidget {
public:
    EditorWidget();

    TextViewWidget* currentWidget() const;
    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void lastIndex();
    size_t getCurrentIndex();
    void addTab(std::string_view tab_name, std::string_view text);
    void removeTab(size_t index);
    void openFile(std::string_view path);

    std::string_view getClassName() const override {
        return "EditorWidget";
    };

private:
    static constexpr int kTabBarHeight = 29 * 2;

    std::shared_ptr<MultiViewWidget<TextViewWidget>> multi_view;
    std::shared_ptr<TabBarWidget> tab_bar;
};

}
