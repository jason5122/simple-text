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
        void closeWindow();

        void onKeyDown(bool temp);

    protected:
        Parent& m_parent;
        std::vector<int> ram_waster;

    private:
        class impl;
        std::unique_ptr<impl> pimpl;
    };

    Parent();
    ~Parent();
    void run();
    Child* createChild();
    void removeChild(Child* child);

protected:
    std::list<Child*> m_children;

    class impl;
    std::unique_ptr<impl> pimpl;
};
