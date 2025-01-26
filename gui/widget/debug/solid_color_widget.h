#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class SolidColorWidget : public Widget {
public:
    SolidColorWidget(const Size& size, const Rgb& color);

    void draw() override;

    constexpr std::string_view class_name() const final override {
        return "SolidColorWidget";
    }

private:
    Rgb color;
};

}  // namespace gui
