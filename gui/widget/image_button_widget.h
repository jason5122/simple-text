#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/button_widget.h"

namespace gui {

class ImageButtonWidget : public ButtonWidget {
public:
    ImageButtonWidget(size_t image_id, const Rgba& text_color, const Rgba& bg_color, int padding);

    void draw() override;

    std::string_view className() const override {
        return "ImageButtonWidget";
    }

private:
    size_t image_id;
    Rgba text_color;
    Rgba bg_color;
    int padding;
};

}  // namespace gui
