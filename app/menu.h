#pragma once

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
    void show(int x, int y) const;

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace app
