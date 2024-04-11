#pragma once

#include "ui/app/app.h"

class EditorWindow : public Parent::Child {
public:
    EditorWindow(Parent& parent) : Child(parent) {}

private:
    void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers);
};
