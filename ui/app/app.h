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

        void onOpenGLActivate(int width, int height) {
            onOpenGLActivateVirtual(width, height);
        }
        void onDraw() {
            onDrawVirtual();
        }
        void onResize(int width, int height) {
            onResizeVirtual(width, height);
        }
        void onScroll(float dx, float dy) {
            onScrollVirtual(dx, dy);
        }
        void onLeftMouseDown(float mouse_x, float mouse_y) {
            onLeftMouseDownVirtual(mouse_x, mouse_y);
        }
        void onLeftMouseDrag(float mouse_x, float mouse_y) {
            onLeftMouseDragVirtual(mouse_x, mouse_y);
        }
        void onKeyDown(app::Key key, app::ModifierKey modifiers) {
            onKeyDownVirtual(key, modifiers);
        }

        // TODO: See if we can make `parent` private as well.
        // private:
        App& parent;

    private:
        class impl;
        std::unique_ptr<impl> pimpl;

        virtual void onOpenGLActivateVirtual(int width, int height) = 0;
        virtual void onDrawVirtual() = 0;
        virtual void onResizeVirtual(int width, int height) = 0;
        virtual void onScrollVirtual(float dx, float dy) = 0;
        virtual void onLeftMouseDownVirtual(float mouse_x, float mouse_y) = 0;
        virtual void onLeftMouseDragVirtual(float mouse_x, float mouse_y) = 0;
        virtual void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers) = 0;
    };

    App();
    void run();
    ~App();

    void onActivate() {
        onActivateVirtual();
    };
    void addWindow(int width, int height) {
        addWindowVirtual(width, height);
    };

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;

    // Non-virtual interface pattern.
    // http://www.gotw.ca/publications/mill18.htm
    virtual void onActivateVirtual() = 0;
    virtual void addWindowVirtual(int width, int height) = 0;
};
