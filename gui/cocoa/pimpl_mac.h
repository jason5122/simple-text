#pragma once

#include "gui/app.h"
#include "gui/cocoa/WindowController.h"
#include "gui/cocoa/displaygl.h"

#import <Cocoa/Cocoa.h>

// TODO: For debugging; remove this.
#include <chrono>
#include <format>
#include <iostream>

namespace gui {

class App::impl {
public:
    NSPoint cascading_point = NSZeroPoint;
    std::unique_ptr<DisplayGL> displaygl;

    impl() : displaygl(DisplayGL::Create()) {}

    // TODO: For debugging; remove this.
    bool has_drawn = false;
    std::chrono::high_resolution_clock::time_point launch_time =
        std::chrono::high_resolution_clock::now();

    // TODO: For debugging; remove this.
    void stopLaunchTimer() {
        if (!has_drawn) {
            has_drawn = true;

            auto draw_time = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(draw_time - launch_time)
                    .count();
            std::cerr << std::format("startup time: {} µs", duration) << '\n';
        }
    }
};

class Window::impl {
public:
    WindowController* window_controller;
};

}
