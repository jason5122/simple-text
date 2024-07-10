#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(const Size& size);

    void draw() override;
    void layout() override;

private:
    static constexpr int tab_width = 360;
    static constexpr int tab_corner_radius = 10;
    static constexpr Size tab_separator_size{.width = 2, .height = 38};

    int tab_index = 0;
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}
