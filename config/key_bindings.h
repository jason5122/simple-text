#pragma once

#include "base/filesystem/file_reader.h"
#include "gui/key.h"
#include "gui/modifier_key.h"

namespace config {
enum class Action {
    Invalid,
    NewWindow,
    CloseWindow,
    NewTab,
    CloseTab,
    PreviousTab,
    NextTab,
    SelectTab1,
    SelectTab2,
    SelectTab3,
    SelectTab4,
    SelectTab5,
    SelectTab6,
    SelectTab7,
    SelectTab8,
    SelectLastTab,
    ToggleSideBar,
};

class KeyBindings {
public:
    KeyBindings();
    Action parseKeyPress(app::Key key, app::ModifierKey modifiers);

private:
    inline static const fs::path key_bindings_path = DataDir() / "key_bindings.json";

    struct JsonSchema {};
};
}
