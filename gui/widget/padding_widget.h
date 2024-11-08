#pragma once

#include "gui/renderer/opengl_types.h"
#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const app::Size& size, const Rgba& color);

    void draw(const std::optional<app::Point>& mouse_pos) override;

    std::string_view getClassName() const override {
        return "PaddingWidget";
    };

private:
    const Rgba color;
};

}  // namespace gui
