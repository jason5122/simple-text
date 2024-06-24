#pragma once

#include "gui/container_widget.h"
#include <memory>

namespace gui {

class HorizontalLayoutWidget : public ContainerWidget {
public:
    HorizontalLayoutWidget(const renderer::Size& size);

    void draw(const renderer::Size& screen_size) override;
    void setPosition(const renderer::Point& position) override;

    void setMainWidget(std::unique_ptr<Widget> widget) override;
    void addChild(std::unique_ptr<Widget> widget) override;
};

}
