#include "editor_window.h"
#include <iostream>

EditorWindow::EditorWindow(Parent& parent) : Child(parent), ram_waster(5000000, 1) {}

EditorWindow::~EditorWindow() {
    std::cerr << "EditorWindow destructor\n";
}

void EditorWindow::onKeyDownVirtual(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createChild();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.destroyChild(this);

        // Another hack to consider: we have the child destroy and delete itself.
        // destroy();
        // delete this;
    }
}
