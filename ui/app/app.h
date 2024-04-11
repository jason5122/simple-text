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
        void createWindow(int width, int height);
        void destroyWindow();

        void onKeyDown(app::Key key, app::ModifierKey modifiers) {
            onKeyDownVirtual(key, modifiers);
        }

    protected:
        Parent& m_parent;

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

protected:
    std::list<Child*> m_children;

    class impl;
    std::unique_ptr<impl> pimpl;
};
