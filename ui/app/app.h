#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <memory>

class App {
public:
    class Window {
    public:
        struct Frame {
            int x;
            int y;
            int width;
            int height;
        };

        Window(App& parent, int width, int height);
        ~Window();
        void show();
        void close();
        void redraw();

        virtual void onOpenGLActivate(int width, int height) {}
        virtual void onDraw() {}
        virtual void onResize(int width, int height) {}
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
