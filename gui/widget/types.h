#pragma once

namespace gui {

enum class MoveBy {
    kCharacters,
    kLines,
    kWords,
};

enum class MoveTo {
    kHardBOL,
    kHardEOL,
    kBOF,
    kEOF,
};

}
