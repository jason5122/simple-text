#include "resizable_widget_window.h"

#include "experiments/resizable_widget/resizable_widget_app.h"
#include "gui/widget/container/vertical_resizing_widget.h"
#include "gui/widget/debug/solid_color_widget.h"
#include "util/profile_util.h"

#include <fmt/base.h>

using namespace gui;

ResizableWidgetWindow::ResizableWidgetWindow(ResizableWidgetApp& parent,
                                             int width,
                                             int height,
                                             int wid)
    : Window(parent, width, height),
      parent(parent),
      main_widget(std::make_unique<VerticalResizingWidget>()) {}

void ResizableWidgetWindow::onOpenGLActivate(const app::Size& size) {
    auto gold_widget = std::make_unique<SolidColorWidget>(size, kGold);
    auto purple_widget = std::make_unique<SolidColorWidget>(app::Size{size.width, 200}, kPurple);
    auto sandy_brown_widget =
        std::make_unique<SolidColorWidget>(app::Size{size.width, 200}, kSandyBrown);
    auto light_blue_widget =
        std::make_unique<SolidColorWidget>(app::Size{size.width, 200}, kLightBlue);
    auto sea_green_widget =
        std::make_unique<SolidColorWidget>(app::Size{size.width, 200}, kSeaGreen);

    main_widget->setMainWidget(std::move(gold_widget));
    main_widget->addChildStart(std::move(purple_widget));
    main_widget->addChildStart(std::move(sandy_brown_widget));
    main_widget->addChildEnd(std::move(light_blue_widget));
    main_widget->addChildEnd(std::move(sea_green_widget));
}

void ResizableWidgetWindow::onDraw(const app::Size& size) {
    main_widget->layout();
    main_widget->draw();

    gui::Renderer::instance().flush(size);
}

void ResizableWidgetWindow::onResize(const app::Size& size) {
    main_widget->setSize(size);
}

void ResizableWidgetWindow::onScroll(const app::Point& mouse_pos, const app::Delta& delta) {
    main_widget->mousePositionChanged(mouse_pos);
    main_widget->scroll(mouse_pos, delta);
    redraw();
}

void ResizableWidgetWindow::onLeftMouseDown(const app::Point& mouse_pos,
                                            app::ModifierKey modifiers,
                                            app::ClickType click_type) {
    dragged_widget = main_widget->widgetAt(mouse_pos);
    if (dragged_widget) {
        fmt::println("dragged_widget = {}", dragged_widget->className());
        dragged_widget->leftMouseDown(mouse_pos, modifiers, click_type);
        redraw();
    } else {
        fmt::println("no widget being dragged");
    }
}

void ResizableWidgetWindow::onLeftMouseUp(const app::Point& mouse_pos) {
    dragged_widget = nullptr;
}

void ResizableWidgetWindow::onLeftMouseDrag(const app::Point& mouse_pos,
                                            app::ModifierKey modifiers,
                                            app::ClickType click_type) {
    if (dragged_widget) {
        dragged_widget->leftMouseDrag(mouse_pos, modifiers, click_type);
        redraw();
    }
}

// This represents the mouse moving *without* being a click+drag.
void ResizableWidgetWindow::onMouseMove(const app::Point& mouse_pos) {
    updateCursorStyle(mouse_pos);

    if (main_widget->mousePositionChanged(mouse_pos)) {
        redraw();
    }
}

void ResizableWidgetWindow::updateCursorStyle(const std::optional<app::Point>& mouse_pos) {
    // Case 1: Dragging operation in progress.
    if (dragged_widget) {
        parent.setCursorStyle(dragged_widget->cursorStyle());
    }
    // Case 2: Mouse position is within window.
    else if (mouse_pos) {
        if (auto hovered_widget = main_widget->widgetAt(mouse_pos.value())) {
            // fmt::println("{}", *hovered_widget);
            parent.setCursorStyle(hovered_widget->cursorStyle());
        }
    }
    // Case 3: Mouse position is outside of window.
    else {
        parent.setCursorStyle(app::CursorStyle::kArrow);
    }
}
