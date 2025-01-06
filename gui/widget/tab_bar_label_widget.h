#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarLabelWidget : public Widget {
public:
    TabBarLabelWidget(size_t font_id,
                      const Size& size,
                      int left_padding = 0,
                      int right_padding = 0);

    void setText(std::string_view str8);
    void setColor(const Rgb& color);
    void addLeftIcon(size_t icon_id);
    void addRightIcon(size_t icon_id);

    void draw() override;

    constexpr std::string_view className() const final override {
        return "TabBarLabelWidget";
    }

private:
    static constexpr Rgb kTempColor{223, 227, 230};
    static constexpr Rgb kFolderIconColor{142, 142, 142};

    size_t font_id;
    int left_padding;
    int right_padding;

    std::string label_str;
    Rgb color{};
    std::vector<size_t> left_side_icons;
    std::vector<size_t> right_side_icons;

    Point centerVertically(int widget_height);
};

}  // namespace gui
