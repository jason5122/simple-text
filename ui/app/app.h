#pragma once

#include <memory>

class App {
public:
    App();
    void run();
    void createNewWindow();
    ~App();

    virtual void onActivate() = 0;
    virtual void onOpenGLActivate() = 0;
    virtual void onDraw() = 0;

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};
