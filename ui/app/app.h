#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <memory>

class App {
public:
    class Window {
    public:
        Window(App& app);
        void createWithSize(int width, int height);
        void redraw();
        void close();
        void quit();
        ~Window();

        virtual void onOpenGLActivate(int width, int height) = 0;
        virtual void onDraw() = 0;
        virtual void onResize(int width, int height) = 0;
        virtual void onScroll(float dx, float dy) = 0;
        virtual void onLeftMouseDown(float mouse_x, float mouse_y) = 0;
        virtual void onLeftMouseDrag(float mouse_x, float mouse_y) = 0;
        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) = 0;

        // TODO: See if we can make `parent` private as well.
        // private:
        App& parent;

    private:
        class impl;
        std::unique_ptr<impl> pimpl;
    };

    App();
    void run();
    ~App();

    virtual void onActivate() = 0;
    virtual void addWindow(int width, int height) = 0;

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;
};
