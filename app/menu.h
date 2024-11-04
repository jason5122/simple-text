#pragma once

#include "app/types.h"
#include <memory>

namespace app {

class Menu {
public:
    Menu();
    virtual ~Menu();

    enum class ItemType {
        kCommand,
        kCheck,
    };

    void addItem(ItemType type);
    void show(const Point& mouse_pos) const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace app
