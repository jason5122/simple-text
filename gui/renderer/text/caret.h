#pragma once

#include "gui/renderer/types.h"
#include <cstddef>

namespace gui {

struct Caret {
    size_t line;
    size_t index;  // UTF-8 index in line.
    Point pos;
};

}
