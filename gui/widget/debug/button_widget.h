#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class ButtonWidget : public Widget {
public:
    ButtonWidget(size_t image_id, Rgba bg_color);

    void draw() override;

    std::string_view className() const override {
        return "ButtonWidget";
    }

private:
    size_t image_id;
    Rgba bg_color;
};

}  // namespace gui
