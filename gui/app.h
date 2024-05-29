#pragma once

#include "gui/key.h"
#include "gui/modifier_key.h"
#include "gui/window.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

// TODO: For debugging; remove this.
#include <chrono>
#include <format>
#include <iostream>

namespace gui {

class App {
public:
    NOT_COPYABLE(App)
    NOT_MOVABLE(App)
    App();
    virtual ~App();
    void run();
    void quit();

    virtual void onLaunch() {}
    virtual void onQuit() {}

private:
    friend class Window;

    class impl;
    std::unique_ptr<impl> pimpl;

    // TODO: For debugging; remove this.
    bool has_drawn = false;
    std::chrono::high_resolution_clock::time_point launch_time =
        std::chrono::high_resolution_clock::now();

    void stopLaunchTimer() {
        // TODO: For debugging; remove this.
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

}
