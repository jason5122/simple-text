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
        // void redraw();
        // void close();
        void quit();
        ~Window();

        void onKeyDown(app::Key key, app::ModifierKey modifiers) {
            onKeyDownVirtual(key, modifiers);
        }

        // TODO: See if we can make `parent` private as well.
        // private:
        App& parent;

    private:
        class impl;
        std::unique_ptr<impl> pimpl;

        virtual void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers) = 0;
    };

    App();
    void run();
    ~App();

    void onActivate() {
        onActivateVirtual();
    };
    void createWindow(int width, int height) {
        createWindowVirtual(width, height);
    };
    void destroyWindow(int idx) {
        destroyWindowVirtual(idx);
    }

private:
    // https://herbsutter.com/gotw/_100/
    class impl;
    std::unique_ptr<impl> pimpl;

    // Non-virtual interface pattern.
    // http://www.gotw.ca/publications/mill18.htm
    virtual void onActivateVirtual() = 0;
    virtual void createWindowVirtual(int width, int height) = 0;
    virtual void destroyWindowVirtual(int idx) = 0;
};
