#pragma once

#include "app/app.h"
#include "app/cocoa/display_gl.h"
#include "app/cocoa/window_controller.h"
#include "app/menu.h"

#include <Cocoa/Cocoa.h>

@interface MenuController : NSObject
- (void)itemSelected:(id)sender;
@end

namespace app {

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

}  // namespace app
