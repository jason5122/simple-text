#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/button_widget.h"

namespace gui {

class ImageButtonWidget : public ButtonWidget {
public:
    ImageButtonWidget(size_t image_id, const Rgb& text_color, const Rgb& bg_color, int padding);

    void draw() override;

    constexpr std::string_view class_name() const final override { return "ImageButtonWidget"; }

private:
    size_t image_id;
    Rgb text_color;
    Rgb bg_color;
    int padding;
};

}  // namespace gui
