#pragma once

#include "gui/container_widget.h"
#include <memory>

namespace gui {

class HorizontalLayoutWidget : public ContainerWidget {
public:
    HorizontalLayoutWidget(const renderer::Size& size);

    void draw(const renderer::Size& screen_size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;
    void leftMouseDown(const renderer::Point& mouse, const renderer::Point& offset) override;
    void leftMouseDrag(const renderer::Point& mouse, const renderer::Point& offset) override;

    void addChild(std::unique_ptr<Widget> widget) override;
};

}
