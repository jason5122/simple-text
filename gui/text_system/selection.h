#pragma once

#include <cstddef>
#include <utility>

namespace gui {

class Selection {
public:
    size_t& start();
    size_t& end();  // TODO: Rename this to avoid collision with range end() function.
    bool empty() const;
    std::pair<size_t, size_t> range() const;
    size_t length() const;
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
    size_t start_caret;
    size_t end_caret;
};

}  // namespace gui
