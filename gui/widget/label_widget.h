#pragma once

#include "base/utf8_string.h"
#include "gui/widget/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget() {}
    LabelWidget(const Size& size) : Widget{size} {}

    void setText(const std::string& str8);
    void addLeftIcon(size_t icon_id);
    void addRightIcon(size_t icon_id);

    void draw() override;

private:
    base::Utf8String label_text{""};
    std::vector<size_t> left_side_icons;
    std::vector<size_t> right_side_icons;

    Point centerVertically(int widget_height);
};

}
