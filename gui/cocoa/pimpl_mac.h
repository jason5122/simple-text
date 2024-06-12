#pragma once

#include "gui/app.h"
#include "gui/cocoa/WindowController.h"
#include "gui/cocoa/display_gl.h"

#import <Cocoa/Cocoa.h>

namespace gui {

class App::impl {
public:
    NSPoint cascading_point = NSZeroPoint;
    std::unique_ptr<DisplayGL> displaygl;

    impl() : displaygl(DisplayGL::Create()) {}
};

class Window::impl {
public:
    WindowController* window_controller;
};

}
