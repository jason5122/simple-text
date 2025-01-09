#pragma once

namespace gui {

// TODO: Combine this with a Sublime-style "commands" enum (e.g., `move`, `insert`).
enum class Action {
    // TODO: Combine movement in a more clean way. Consider moving away from an enum and into a
    // struct if we need additional info (e.g., `move` by lines). It's slower, but we already use
    // structs for other types anyways.
    kMoveForwardByCharacters,
    kMoveBackwardByCharacters,
    kMoveForwardByLines,
    kMoveBackwardByLines,
    kMoveForwardByWords,
    kMoveBackwardByWords,
    kMoveToBOL,
    kMoveToEOL,
    kMoveToHardBOL,
    kMoveToHardEOL,
    kMoveToBOF,
    kMoveToEOF,
    kLeftDelete,
    kRightDelete,
    kDeleteWordForward,
    kDeleteWordBackward,
    kInsertNewline,
    kInsertNewlineIgnoringFieldEditor,
    kInsertTab,
};

// TODO: Combine this with a Sublime-style "commands" enum (e.g., `new_file`, `new_window`).
enum class AppAction {
    kNewFile,
    kNewWindow,
};

}  // namespace gui
