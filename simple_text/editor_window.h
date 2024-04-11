#pragma once

#include "simple_text/simple_text.h"
#include "ui/app/app.h"

class EditorWindow : public Parent::Child {
public:
    EditorWindow(SimpleText& parent, int width, int height);
    void onKeyDown(app::Key key, app::ModifierKey modifiers) override;

private:
    SimpleText& parent;
    std::vector<int> ram_waster;
};
