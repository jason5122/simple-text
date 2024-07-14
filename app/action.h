#pragma once

namespace app {

enum class Action {
    // TODO: Combine movement in a more clean way.
    kMoveForwardByCharacter,
    kMoveBackwardByCharacter,
    kMoveToHardBOL,
    kMoveToHardEOL,
    kLeftDelete,
};

}
