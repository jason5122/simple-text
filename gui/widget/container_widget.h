#pragma once

#include "gui/widget/widget.h"

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget() = default;
    ContainerWidget(const Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    Widget* getWidgetAtPosition(const Point& pos) override = 0;
};

}
