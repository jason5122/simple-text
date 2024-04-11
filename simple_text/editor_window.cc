#include "editor_window.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height)
    : Child(parent, width, height), parent(parent), ram_waster(5000000, 1) {}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createChild();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.destroyChild(this);
    }
}
