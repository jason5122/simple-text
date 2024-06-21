#pragma once

#include "gui/widget.h"
#include <memory>
#include <vector>

namespace gui {

class ContainerWidget : public Widget {
public:
    ContainerWidget(const renderer::Size& size) : Widget{size} {}
    virtual ~ContainerWidget() {}

    virtual void addChild(std::unique_ptr<Widget> widget) = 0;

protected:
    std::vector<std::unique_ptr<Widget>> children;
};

}
