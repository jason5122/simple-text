#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const Size& size);

    void draw(const std::optional<Point>& mouse_pos) override;
    void layout() override;

    Widget* getWidgetAtPosition(const Point& pos) override {
        return hitTest(pos) ? this : nullptr;
    }
    std::string_view getClassName() const override {
        return "StatusBarWidget";
    };

private:
    static constexpr Point kLeftPadding{32, 0};
    static constexpr Rgba kStatusBarColor{199, 203, 209, 255};

    std::unique_ptr<LabelWidget> line_column_label;
};

}
