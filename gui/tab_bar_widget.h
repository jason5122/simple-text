#pragma once

#include "gui/label_widget.h"
#include "gui/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(const renderer::Size& size);

    void draw() override;
    void layout() override;

private:
    static constexpr int tab_width = 360;
    static constexpr int tab_corner_radius = 10;
    static constexpr renderer::Size tab_separator_size{.width = 2, .height = 38};

    int tab_index = 0;
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}
