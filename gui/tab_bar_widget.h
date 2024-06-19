#pragma once

#include "gui/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(std::shared_ptr<renderer::Renderer> renderer, int tab_bar_height);

    void draw(int width, int height) override;
    void scroll(int dx, int dy) override;

private:
    int tab_bar_height;
};

}
