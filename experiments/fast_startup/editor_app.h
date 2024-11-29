#pragma once

#include "app/app.h"
#include "experiments/fast_startup/editor_window.h"

#include <vector>

class EditorApp : public app::App {
public:
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;

private:
    // std::string a;
    size_t a [[maybe_unused]];
    size_t b [[maybe_unused]];
    size_t c [[maybe_unused]];
    std::unique_ptr<EditorWindow> win;
};
