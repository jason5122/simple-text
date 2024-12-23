#pragma once

#include "app/window.h"

#include "gui/widget/debug/solid_color_widget.h"

class ResizableWidgetApp;

class ResizableWidgetWindow : public app::Window {
public:
    ResizableWidgetWindow(ResizableWidgetApp& parent, int width, int height, int wid);

    void onOpenGLActivate(const app::Size& size) override;
    void onDraw(const app::Size& size) override;
    void onResize(const app::Size& size) override;
    void onScroll(const app::Point& mouse_pos, const app::Delta& delta) override;

private:
    ResizableWidgetApp& parent [[maybe_unused]];

    std::shared_ptr<gui::SolidColorWidget> main_widget;
};
