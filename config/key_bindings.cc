#include "gui/modifier_key.h"
#include "key_bindings.h"
#include <iostream>

namespace config {
KeyBindings::KeyBindings() {
    reload();
}

void KeyBindings::reload() {
    std::vector<JsonSchema> schema = {
        {
            .keys = "primary+shift+n",
            .command = "new_window",
        },
        {
            .keys = "primary+shift+w",
            .command = "close_window",
        },
        {
            .keys = "primary+n",
            .command = "new_tab",
        },
        {
            .keys = "primary+w",
            .command = "close_tab",
        },
    };

    std::string buffer;
    if (fs::exists(kKeyBindingsPath)) {
        // TODO: Handle errors in a better way.
        glz::parse_error error = glz::read_file_json(schema, kKeyBindingsPath.string(), buffer);
        if (error) {
            std::cerr << glz::format_error(error, buffer) << '\n';
        }
    } else {
        std::filesystem::create_directory(kKeyBindingsPath.parent_path());
        glz::write_error error =
            glz::write_file_json<kDefaultOptions>(schema, kKeyBindingsPath.string(), buffer);

        if (error) {
            std::cerr << "Could not write color scheme to " << kKeyBindingsPath << ".\n";
        }
    }

    bindings.clear();
    for (const auto& binding : schema) {
        addBinding(binding.keys, binding.command);
    }
}

Action KeyBindings::parseKeyPress(app::Key key, app::ModifierKey modifiers) {
    for (const auto& binding : bindings) {
        if (key == binding.key && modifiers == binding.modifiers) {
            return binding.action;
        }
    }

    // if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift))
    // {
    //     return Action::kNewWindow;
    // }
    // if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift))
    // {
    //     return Action::kCloseWindow;
    // }
    // if (key == app::Key::kN && modifiers == app::kPrimaryModifier) {
    //     return Action::kNewTab;
    // }
    // if (key == app::Key::kW && modifiers == app::kPrimaryModifier) {
    //     return Action::kCloseTab;
    // }
    // if (key == app::Key::kJ && modifiers == app::kPrimaryModifier) {
    //     return Action::kPreviousTab;
    // }
    // if (key == app::Key::kK && modifiers == app::kPrimaryModifier) {
    //     return Action::kNextTab;
    // }
    // if (key == app::Key::k1 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab1;
    // }
    // if (key == app::Key::k2 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab2;
    // }
    // if (key == app::Key::k3 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab3;
    // }
    // if (key == app::Key::k4 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab4;
    // }
    // if (key == app::Key::k5 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab5;
    // }
    // if (key == app::Key::k6 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab6;
    // }
    // if (key == app::Key::k7 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab7;
    // }
    // if (key == app::Key::k8 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectTab8;
    // }
    // if (key == app::Key::k9 && modifiers == app::kPrimaryModifier) {
    //     return Action::kSelectLastTab;
    // }
    // if (key == app::Key::k0 && modifiers == app::kPrimaryModifier) {
    //     return Action::kToggleSideBar;
    // }
    return Action::kNone;
}

void KeyBindings::addBinding(const std::string& keys, const std::string& command) {
    app::Key key = app::Key::kNone;
    app::ModifierKey modifiers = app::ModifierKey::kNone;
    Action action = Action::kNone;

    auto parse_substring = [&key, &modifiers](const std::string& substr) -> bool {
        if (substr.length() == 1 && 'a' <= substr[0] && substr[0] <= 'z') {
            int offset = substr[0] - 'a';
            key = static_cast<app::Key>(static_cast<int>(app::Key::kA) + offset);
        } else if (modifier_map.contains(substr)) {
            modifiers |= modifier_map.at(substr);
        } else {
            return false;
        }
        return true;
    };

    size_t last = 0;
    size_t next = 0;
    std::string substr;
    while ((next = keys.find(kDelimiter, last)) != std::string::npos) {
        substr = keys.substr(last, next - last);
        if (!parse_substring(substr)) {
            std::cerr << "KeyBindings::addBinding() error: Invalid key.\n";
            return;
        }
        last = next + 1;
    }
    substr = keys.substr(last);
    if (!parse_substring(substr)) {
        std::cerr << "KeyBindings::addBinding() error: Invalid key.\n";
        return;
    }
    if (key == app::Key::kNone) {
        std::cerr << "KeyBindings::addBinding() error: No key provided.\n";
        return;
    }

    if (action_map.contains(command)) {
        action = action_map.at(command);
    } else {
        std::cerr << "KeyBindings::addBinding() error: Invalid command.\n";
        return;
    }

    bindings.emplace_back(key, modifiers, action);
}
}
