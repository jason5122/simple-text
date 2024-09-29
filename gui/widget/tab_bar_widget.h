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
    void lastIndex();
    void addTab(std::string_view title);
    void removeTab(size_t index);

    void draw(const std::optional<Point>& mouse_pos) override;
    void layout() override;

private:
    static constexpr int kTabWidth = 360;
    static constexpr int kTabCornerRadius = 10;
    static constexpr Size kTabSeparatorSize{.width = 2, .height = 38};
    static constexpr Rgba kTabBarColor{190, 190, 190, 255};
    static constexpr Rgba kTabColor{253, 253, 253, 255};
    static constexpr Rgba kTabSeparatorColor{148, 149, 149, 255};
    static constexpr Rgb kTabTextColor{92, 92, 92};

    size_t index = 0;
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}
