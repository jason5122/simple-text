#include "app/modifier_key.h"
#include "key_bindings.h"

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace config {
KeyBindings::KeyBindings() {
    kDefaultKeyBindingsSchema = {
        {.keys = "primary+q", .command = "exit"},
        {.keys = "primary+shift+n", .command = "new_window"},
        {.keys = "primary+shift+w", .command = "close_window"},
        {.keys = "primary+n", .command = "new_tab"},
        {.keys = "primary+w", .command = "close_tab"},
        {.keys = "primary+j", .command = "previous_tab"},
        {.keys = "primary+k", .command = "next_tab"},
        {.keys = "primary+1", .command = "select_tab_1"},
        {.keys = "primary+2", .command = "select_tab_2"},
        {.keys = "primary+3", .command = "select_tab_3"},
        {.keys = "primary+4", .command = "select_tab_4"},
        {.keys = "primary+5", .command = "select_tab_5"},
        {.keys = "primary+6", .command = "select_tab_6"},
        {.keys = "primary+7", .command = "select_tab_7"},
        {.keys = "primary+8", .command = "select_tab_8"},
        {.keys = "primary+9", .command = "select_last_tab"},
        {.keys = "primary+0", .command = "toggle_side_bar"},
    };

    reload();
}

void KeyBindings::reload() {
    std::vector<JsonSchema>& schema = kDefaultKeyBindingsSchema;

    std::string buffer;
    if (fs::exists(kKeyBindingsPath)) {
        // TODO: Handle errors in a better way.
        glz::parse_error error = glz::read_file_json(schema, kKeyBindingsPath.string(), buffer);
        if (error) {
            std::println(glz::format_error(error, buffer));
        }
    } else {
        std::filesystem::create_directory(kKeyBindingsPath.parent_path());
        glz::write_error error =
            glz::write_file_json<kDefaultOptions>(schema, kKeyBindingsPath.string(), buffer);

        if (error) {
            std::println("Could not write key bindings to {}.", kKeyBindingsPath);
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
        } else if (substr.length() == 1 && '0' <= substr[0] && substr[0] <= '9') {
            int offset = substr[0] - '0';
            key = static_cast<app::Key>(static_cast<int>(app::Key::k0) + offset);
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
            std::println("KeyBindings::addBinding() error: Invalid key.");
            return;
        }
        last = next + 1;
    }
    substr = keys.substr(last);
    if (!parse_substring(substr)) {
        std::println("KeyBindings::addBinding() error: Invalid key.");
        return;
    }
    if (key == app::Key::kNone) {
        std::println("KeyBindings::addBinding() error: No key provided.");
        return;
    }

    if (action_map.contains(command)) {
        action = action_map.at(command);
    } else {
        std::println("KeyBindings::addBinding() error: Invalid command.");
        return;
    }

    bindings.emplace_back(key, modifiers, action);
}
}
