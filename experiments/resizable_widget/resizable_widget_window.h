#pragma once

#include "app/window.h"

#include "gui/widget/custom_widget.h"

class FastStartupApp;

class FastStartupWindow : public app::Window {
public:
    FastStartupWindow(FastStartupApp& parent, int width, int height, int wid);

    void onOpenGLActivate(const app::Size& size) override;
    void onDraw(const app::Size& size) override;
    void onResize(const app::Size& size) override;
    void onScroll(const app::Point& mouse_pos, const app::Delta& delta) override;

private:
    FastStartupApp& parent [[maybe_unused]];

    std::shared_ptr<gui::CustomWidget> main_widget;
};
