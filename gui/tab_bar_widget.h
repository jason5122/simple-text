#pragma once

#include "gui/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(std::shared_ptr<renderer::Renderer> renderer, int tab_bar_height);

    void draw(const renderer::Size& size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;

private:
    int tab_bar_height;
};

}
