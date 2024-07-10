#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const renderer::Size& size);

    void draw() override;
    void layout() override;

private:
    std::unique_ptr<LabelWidget> line_column_label;
};

}
