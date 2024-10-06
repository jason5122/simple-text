#pragma once

#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const Size& size, const Rgba& color);

    void draw(const std::optional<Point>& mouse_pos) override;

    Widget* getWidgetAtPosition(const Point& pos) override {
        return hitTest(pos) ? this : nullptr;
    }
    std::string_view getClassName() const override {
        return "PaddingWidget";
    };

private:
    const Rgba color;
};

}
