#pragma once

#include "gui/widget.h"
#include <memory>
#include <vector>

namespace gui {

class MainWidget : public Widget {
public:
    MainWidget(std::shared_ptr<renderer::Renderer> renderer);

    void draw(int width, int height) override;
    void scroll(int dx, int dy) override;

    void addChild(std::unique_ptr<Widget> widget);

private:
    std::vector<std::unique_ptr<Widget>> children;
};

}
