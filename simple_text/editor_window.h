#pragma once

#include "ui/app/app.h"

class EditorWindow : public Parent::Child {
    void onKeyDown(app::Key key, app::ModifierKey modifiers) {
        if (key == app::Key::kN &&
            modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
            m_parent.createChild();
        }
        if (key == app::Key::kW &&
            modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
            m_parent.destroyChild(this);
        }
    }
};
