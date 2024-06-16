#pragma once

#include "app/gui_action.h"
#include "app/key.h"
#include "app/modifier_key.h"
#include "app/window.h"
#include "util/non_copyable.h"
#include <memory>

// TODO: For debugging; remove this.
#include <chrono>
#include <format>
#include <iostream>

namespace app {

class App {
public:
    App();
    virtual ~App();
    void run();
    void quit();

    virtual void onLaunch() {}
    virtual void onQuit() {}
    virtual void onGuiAction(GuiAction action) {}

    // TODO: Find a way to make this private! We currently need this for GTK's callbacks.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    friend class Window;

    // class impl;
    // std::unique_ptr<impl> pimpl;

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
