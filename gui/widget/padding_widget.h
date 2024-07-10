#pragma once

#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const Size& size, const Rgba& color);

    void draw() override;

private:
    const Rgba color;
};

}
