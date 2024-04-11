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

    protected:
        Parent& parent;

    private:
        std::vector<int> ram_waster;

        class impl;
        std::unique_ptr<impl> pimpl;
    };

    Parent();
    ~Parent();
    void run();

    virtual void onActivate() {}
    virtual void createChild() {}
    virtual void destroyChild(Child* child) {}

protected:
    class impl;
    std::unique_ptr<impl> pimpl;
};
