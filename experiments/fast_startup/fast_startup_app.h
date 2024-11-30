#pragma once

#include "app/app.h"
#include "experiments/fast_startup/fast_startup_window.h"

#include <memory>
#include <vector>

class FastStartupApp : public app::App {
public:
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;

private:
    friend class FastStartupWindow;

    std::vector<std::unique_ptr<FastStartupWindow>> editor_windows;
};
