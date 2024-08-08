#pragma once

#include "app/app.h"
#include "app/cocoa/WindowController.h"
#include "app/cocoa/display_gl.h"

#import <Cocoa/Cocoa.h>

namespace app {

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
