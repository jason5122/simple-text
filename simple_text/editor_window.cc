#include "editor_window.h"

EditorWindow::EditorWindow(SimpleText& parent, int x, int y, int width, int height)
    : Window(parent, x, y, width, height), parent(parent) {}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        Frame frame = getFrame();
        float titlebar_height = getTitlebarHeight();
        parent.createChild(frame.x + titlebar_height, frame.y - titlebar_height);
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.destroyChild(this);
    }
}
