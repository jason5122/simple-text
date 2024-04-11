#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <memory>
#include <vector>

class Parent {
public:
    class Child {
    public:
        Child(Parent& parent, int width, int height);
        ~Child();
        void show();
        void close();

        virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}

    private:
        Parent& parent;
        std::vector<int> ram_waster;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    Parent();
    ~Parent();
    void run();

    virtual void onLaunch() {}

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};
