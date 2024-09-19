#pragma once

namespace gui {

enum class MoveBy {
    kCharacters,
    kLines,
    kWords,
};

enum class MoveTo {
    kBOL,
    kEOL,
    kHardBOL,
    kHardEOL,
    kBOF,
    kEOF,
};

}
