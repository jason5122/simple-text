#pragma once

#include "gui/widget.h"
#include <memory>
#include <vector>

namespace gui {

class HorizontalLayoutWidget : public Widget {
public:
    HorizontalLayoutWidget(std::shared_ptr<renderer::Renderer> renderer);

    void draw(const renderer::Size& size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;

    void addChild(std::unique_ptr<Widget> widget);

private:
    std::vector<std::unique_ptr<Widget>> children;
};

}
