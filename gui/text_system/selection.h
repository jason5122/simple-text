#pragma once

#include "gui/text_system/caret.h"

namespace gui {

class Selection {
public:
    Caret& start();
    Caret& end();
    bool empty() const;
    void setIndex(size_t index, bool extend);
    std::pair<size_t, size_t> range() const;

private:
    Caret start_caret{};
    Caret end_caret{};
};

}
