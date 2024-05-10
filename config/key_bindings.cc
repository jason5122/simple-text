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
}
