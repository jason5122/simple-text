#include "key_bindings.h"

namespace config {
KeyBindings::KeyBindings() {}

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
    return Action::Invalid;
}
}
