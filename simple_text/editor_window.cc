#include "build/buildflag.h"
#include "editor_window.h"
#include "util/profile_util.h"
#include <iostream>

void EditorWindow::onKeyDownVirtual(app::Key key, app::ModifierKey modifiers) {
    using app::Any;

    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        std::cerr << "create window\n";
        parent.createWindow(600, 400);
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        std::cerr << "destroy window\n";
        parent.destroyWindow(0);
    }
}

EditorWindow::~EditorWindow() {
    std::cerr << "editor window destructor\n";
    delete temp;
}
