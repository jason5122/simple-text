#pragma once

#include "base/filesystem/file_reader.h"
#include "glaze/glaze.hpp"
#include "gui/key.h"
#include "gui/modifier_key.h"
#include <vector>

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
    void reload();
    Action parseKeyPress(app::Key key, app::ModifierKey modifiers);

private:
    inline static const fs::path kKeyBindingsPath = DataDir() / "key_bindings.json";
    static constexpr glz::opts kDefaultOptions{.prettify = true, .indentation_width = 2};
    static constexpr char kDelimiter = '+';

    struct JsonSchema {
        std::string keys;
        std::string command;
    };

    struct Binding {
        app::Key key;
        app::ModifierKey modifiers;
        Action action;
    };

    std::vector<Binding> bindings;
    void addBinding(const std::string& keys, const std::string& command);
};
}
