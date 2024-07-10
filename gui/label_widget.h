#pragma once

#include "base/utf8_string.h"
#include "gui/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget() {}
    LabelWidget(const renderer::Size& size) : Widget{size} {}

    void setText(const std::string& str8);
    void addIcon(size_t icon_id);

    void draw() override;

private:
    base::Utf8String label_text{""};
    std::vector<size_t> left_side_icons;

    renderer::Point centerVertically(int widget_height);
};

}
