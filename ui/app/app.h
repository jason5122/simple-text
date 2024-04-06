#pragma once

#include "ui/app/app_window.h"
#include <memory>

class App {
public:
    App();
    void run();
    void createNewWindow(AppWindow& app_window, int width, int height);
    ~App();

    virtual void onActivate() = 0;

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};
