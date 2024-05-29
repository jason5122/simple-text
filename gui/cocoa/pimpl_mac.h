#pragma once

#include "gui/app.h"
#include "gui/cocoa/WindowController.h"
#include "gui/cocoa/displaygl.h"

#import <Cocoa/Cocoa.h>

namespace gui {

class App::impl {
public:
    NSApplication* ns_app;
    NSPoint cascading_point = NSZeroPoint;
    std::unique_ptr<DisplayGL> displaygl;

    impl() : displaygl(DisplayGL::Create()) {}
};

class Window::impl {
public:
    WindowController* window_controller;
};

}
