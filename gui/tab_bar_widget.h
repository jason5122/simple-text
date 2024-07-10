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
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}
