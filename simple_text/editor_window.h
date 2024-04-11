#pragma once

#include "ui/app/app.h"

class EditorWindow : public Parent::Child {
public:
    EditorWindow(Parent& parent, int width, int height);
    void onKeyDown(app::Key key, app::ModifierKey modifiers) override;

private:
    std::vector<int> ram_waster;
};
