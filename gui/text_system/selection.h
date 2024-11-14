#pragma once

#include "gui/text_system/caret.h"

namespace gui {

class Selection {
public:
    Caret& start();
    Caret& end();  // TODO: Rename this to avoid collision with range end() function.
    bool empty() const;
    std::pair<size_t, size_t> range() const;
    void setIndex(size_t index, bool extend);
    void setRange(size_t start_index, size_t end_index);
    void increment(size_t inc, bool extend);
    void decrement(size_t dec, bool extend);

    enum class Direction {
        kLeft,
        kRight,
    };
    void collapse(Direction direction);

private:
    Caret start_caret{};
    Caret end_caret{};
};

}  // namespace gui
