#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget(size_t font_id, const Rgb& color, int left_padding = 0, int right_padding = 0);

    void setText(std::string_view str8);

    void draw() override;

    constexpr std::string_view className() const final override {
        return "LabelWidget";
    }

private:
    static constexpr Rgba kTempColor{223, 227, 230, 255};
    static constexpr Rgba kFolderIconColor{142, 142, 142, false};

    size_t font_id;

    std::string label_str;
    Rgb color;

    app::Point centerVertically(int widget_height);
};

}  // namespace gui
