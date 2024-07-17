#pragma once

namespace app {

enum class Action {
    // TODO: Combine movement in a more clean way.
    kMoveForwardByCharacters,
    kMoveBackwardByCharacters,
    kMoveForwardByLines,
    kMoveBackwardByLines,
    kMoveToHardBOL,
    kMoveToHardEOL,
    kMoveToBOF,
    kMoveToEOF,
    kLeftDelete,
    kInsertNewline,
    kInsertTab,
};

}
