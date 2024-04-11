#pragma once

#include "ui/app/app.h"

class EditorWindow : public Parent::Child {
public:
    EditorWindow(Parent& parent);

private:
    std::vector<int> ram_waster;

    void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers);
};
