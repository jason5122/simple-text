#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const app::Size& size, const Rgba& color);

    void draw() override;

    std::string_view className() const override {
        return "PaddingWidget";
    }

private:
    const Rgba color;
};

}  // namespace gui
