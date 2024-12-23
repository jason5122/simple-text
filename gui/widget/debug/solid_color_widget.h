#pragma once

#include "gui/renderer/opengl_types.h"
#include "gui/widget/widget.h"

namespace gui {

class SolidColorWidget : public Widget {
public:
    SolidColorWidget(const app::Size& size, const Rgba& color);

    void draw() override;

    std::string_view className() const override {
        return "SolidColorWidget";
    }

private:
    Rgba color;
};

}  // namespace gui
