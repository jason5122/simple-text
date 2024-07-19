#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(int height);

    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void addTab(const std::string& title);
    void removeTab(size_t index);

    void draw() override;
    void layout() override;

private:
    static constexpr int kTabWidth = 360;
    static constexpr int kTabCornerRadius = 10;
    static constexpr Size kTabSeparatorSize{.width = 2, .height = 38};

    int index = 0;
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}