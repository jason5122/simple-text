#pragma once

#include "app/types.h"
#include <memory>
#include <optional>

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
    std::optional<size_t> show(const Point& mouse_pos) const;
    void setSelectedIndex(size_t index);

private:
    size_t index;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace app
