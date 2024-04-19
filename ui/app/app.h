#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <memory>

class App {
public:
    class Window {
    public:
        Window(App& parent, int width, int height);
        ~Window();
        void show();
        void close();
        void redraw();

        virtual void onOpenGLActivate(int width, int height) {}
        virtual void onDraw() {}
        virtual void onResize(int width, int height) {}
        virtual void onScroll(float dx, float dy) {}
        virtual void onLeftMouseDown(float mouse_x, float mouse_y) {}
        virtual void onLeftMouseDrag(float mouse_x, float mouse_y) {}
        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}

    private:
        App& parent;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    App();
    ~App();
    void run();

    virtual void onLaunch() {}

    // TODO: Debug; remove this.
    void incrementWindowCount();

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};
