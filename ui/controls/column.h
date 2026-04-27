#pragma once

#include "ui/view.h"

namespace ui {

class Column final : public View {
public:
    explicit Column(float padding = 24.0f, float spacing = 12.0f);

    Size preferred_size(Size available) const override;
    void layout(Rect bounds) override;
    void paint(PaintContext& context) override;

private:
    float padding_ = 0;
    float spacing_ = 0;
};

}  // namespace ui
