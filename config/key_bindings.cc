#include "key_bindings.h"
#include <iostream>

namespace config {
KeyBindings::KeyBindings() {
    reload();
}

void KeyBindings::reload() {
    std::vector<JsonSchema> schema = {{
        .keys = "primary+,",
        .command = "edit_settings",
    }};

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

    for (const auto& binding : schema) {
        fprintf(stderr, "key: \"%s\", command: \"%s\"\n", &binding.keys[0], &binding.command[0]);
        addBinding(binding.keys, binding.command);
    }
}

Action KeyBindings::parseKeyPress(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        return Action::NewWindow;
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        return Action::CloseWindow;
    }
    if (key == app::Key::kN && modifiers == app::kPrimaryModifier) {
        return Action::NewTab;
    }
    if (key == app::Key::kW && modifiers == app::kPrimaryModifier) {
        return Action::CloseTab;
    }
    if (key == app::Key::kJ && modifiers == app::kPrimaryModifier) {
        return Action::PreviousTab;
    }
    if (key == app::Key::kK && modifiers == app::kPrimaryModifier) {
        return Action::NextTab;
    }
    if (key == app::Key::k1 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab1;
    }
    if (key == app::Key::k2 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab2;
    }
    if (key == app::Key::k3 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab3;
    }
    if (key == app::Key::k4 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab4;
    }
    if (key == app::Key::k5 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab5;
    }
    if (key == app::Key::k6 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab6;
    }
    if (key == app::Key::k7 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab7;
    }
    if (key == app::Key::k8 && modifiers == app::kPrimaryModifier) {
        return Action::SelectTab8;
    }
    if (key == app::Key::k9 && modifiers == app::kPrimaryModifier) {
        return Action::SelectLastTab;
    }
    if (key == app::Key::k0 && modifiers == app::kPrimaryModifier) {
        return Action::ToggleSideBar;
    }
    return Action::Invalid;
}

void KeyBindings::addBinding(const std::string& keys, const std::string& command) {
    size_t last = 0;
    size_t next = 0;
    while ((next = keys.find(kDelimiter, last)) != std::string::npos) {
        std::cerr << keys.substr(last, next - last) << '\n';
        last = next + 1;
    }
    std::cerr << keys.substr(last) << '\n';
}
}
