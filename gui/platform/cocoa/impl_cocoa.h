#pragma once

#include "gui/platform/app.h"
#include "gui/platform/cocoa/display_gl.h"
#include "gui/platform/cocoa/window_controller.h"
#include "gui/platform/menu.h"
#include <Cocoa/Cocoa.h>

@interface MenuController : NSObject
- (void)itemSelected:(id)sender;
@end

namespace gui {

class App::Impl {
public:
    NSPoint cascading_point = NSZeroPoint;
    std::unique_ptr<DisplayGL> display_gl;

    Impl() : display_gl(DisplayGL::Create()) {}
};

class WindowWidget::Impl {
public:
    WindowController* window_controller;
};

class Menu::Impl {
public:
    NSMenu* ns_menu;
    MenuController* menu_controller;
};

}  // namespace gui
