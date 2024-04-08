#pragma once

#include "ui/app/app_window.h"
#include <memory>

class App {
public:
    class Window {
    public:
        Window(App& app);
        void createWithSize(int width, int height);
        ~Window() = default;

    private:
        App& parent;

        class impl;
        // TODO: Figure out how to use unique_ptr here.
        impl* pimpl;
    };

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
