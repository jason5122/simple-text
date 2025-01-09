#pragma once

#include "gui/types.h"

#include <memory>
#include <optional>
#include <string_view>

namespace gui {

class Menu {
public:
    Menu();
    virtual ~Menu();

    void addItem(std::string_view label);
    std::optional<size_t> show(const Point& mouse_pos) const;
    void setSelectedIndex(size_t index);

private:
    size_t index;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace gui
