#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class PaddingWidget : public Widget {
public:
    PaddingWidget(const Size& size, const Rgb& color);

    void draw() override;

    constexpr std::string_view class_name() const final override { return "PaddingWidget"; }

private:
    const Rgb color;
};

}  // namespace gui
