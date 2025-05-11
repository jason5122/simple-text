#pragma once

#include "gui/platform/window_widget.h"
#include "gui/widget/debug/solid_color_widget.h"

namespace gui {

class FastStartupApp;

class FastStartupWindow : public WindowWidget {
public:
    FastStartupWindow(FastStartupApp& parent, int width, int height, int wid);

    // Widget overrides.
    void draw() override;
    void layout() override;

    constexpr std::string_view class_name() const final override { return "FastStartupWindow"; }
};

}  // namespace gui
