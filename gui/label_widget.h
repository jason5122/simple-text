#pragma once

#include "base/utf8_string.h"
#include "gui/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget() {}
    LabelWidget(const renderer::Size& size) : Widget{size} {}

    void setText(const std::string& str8);

    void draw() override;

private:
    base::Utf8String label_text{""};
};

}
