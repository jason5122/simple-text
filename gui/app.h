#pragma once

#include "gui/key.h"
#include "gui/modifier_key.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

class App {
public:
    class Window {
    public:
        NOT_COPYABLE(Window)
        NOT_MOVABLE(Window)
        Window(App& parent, int width, int height);
        virtual ~Window();
        void show();
        void close();
        void redraw();

        int width();
        int height();
        int scaleFactor();
        bool isDarkMode();

        virtual void onOpenGLActivate(int width, int height) {}
        virtual void onDraw() {}
        virtual void onResize(int width, int height) {}
        virtual void onScroll(int dx, int dy) {}
        virtual void onLeftMouseDown(int mouse_x, int mouse_y, app::ModifierKey modifiers) {}
        virtual void onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {}
        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}
        virtual void onClose() {}
        virtual void onDarkModeToggle() {}

    private:
        App& parent;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    NOT_COPYABLE(App)
    NOT_MOVABLE(App)
    App();
    virtual ~App();
    void run();
    void quit();

    virtual void onLaunch() {}

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};
