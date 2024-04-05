#pragma once

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
    impl* pimpl;
    // TODO: Learn and use std::unique_ptr instead.
    // https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile
    // std::unique_ptr<impl> pimpl;
};
