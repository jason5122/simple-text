#pragma once

#include "gui/app/app.h"
#include "gui/app/cocoa/display_gl.h"
#include "gui/app/cocoa/window_controller.h"
#include "gui/app/menu.h"

#include <Cocoa/Cocoa.h>

@interface MenuController : NSObject
- (void)itemSelected:(id)sender;
@end

namespace gui {

class App::impl {
public:
    NSPoint cascading_point = NSZeroPoint;
    std::unique_ptr<DisplayGL> display_gl;

    impl() : display_gl(DisplayGL::Create()) {}
};

class Window::impl {
public:
    WindowController* window_controller;
};

class Menu::impl {
public:
    NSMenu* ns_menu;
    MenuController* menu_controller;
};

}  // namespace gui
