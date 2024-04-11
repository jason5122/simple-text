#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <memory>
#include <vector>

class App {
public:
    class Window {
    public:
        Window(App& parent, int width, int height);
        ~Window();
        void show();
        void close();

        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}

    private:
        App& parent;
        std::vector<int> ram_waster;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    App();
    ~App();
    void run();

    virtual void onLaunch() {}

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};
