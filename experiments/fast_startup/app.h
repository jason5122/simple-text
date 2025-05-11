#pragma once

#include "experiments/fast_startup/window.h"
#include "gui/platform/app.h"

#include <memory>
#include <vector>

namespace gui {

class FastStartupApp : public App {
public:
    void create_window();
    void destroy_window(int wid);

    void on_launch() override;

private:
    friend class FastStartupWindow;

    std::vector<std::unique_ptr<FastStartupWindow>> windows;
};

}  // namespace gui
