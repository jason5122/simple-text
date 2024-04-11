#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"
#include <list>
#include <memory>

class Parent {
public:
    class Child {
    public:
        Child(Parent& parent);
        ~Child();
        void create(int width, int height);
        void destroy();

        // Non-virtual interface pattern.
        // http://www.gotw.ca/publications/mill18.htm
        void onKeyDown(app::Key key, app::ModifierKey modifiers) {
            onKeyDownVirtual(key, modifiers);
        }

    protected:
        Parent& parent;

    private:
        std::vector<int> ram_waster;

        class impl;
        std::unique_ptr<impl> pimpl;

        virtual void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers);
    };

    Parent();
    ~Parent();
    void run();
    Child* createChild();
    void destroyChild(Child* child);

    void onActivate() {
        onActivateVirtual();
    };

protected:
    class impl;
    std::unique_ptr<impl> pimpl;

    virtual void onActivateVirtual() {}
};
