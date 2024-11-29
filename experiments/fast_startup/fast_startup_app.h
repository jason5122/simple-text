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

    size_t kPanelClose2xIndex;
    size_t kFolderOpen2xIndex;
    size_t kStanfordBunny;
    size_t kDice;
    size_t kExampleJpg;
    size_t kLCD;

    std::vector<std::unique_ptr<FastStartupWindow>> editor_windows;
};
