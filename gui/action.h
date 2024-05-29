#pragma once

namespace gui {

enum class Action {
    kNone,
    // TODO: Combine movement in a more clean way.
    kMoveForwardByCharacter,
    kMoveBackwardByCharacter,
    kLeftDelete,
};

}
