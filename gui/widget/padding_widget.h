#pragma once

#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const renderer::Size& size, const renderer::Rgba& color);

    void draw() override;

private:
    const renderer::Rgba color;
};

}
