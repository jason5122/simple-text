#pragma once

#include "simple_text/simple_text.h"
#include "ui/app/app.h"

class EditorWindow : public App::Window {
public:
    EditorWindow(SimpleText& parent, int x, int y, int width, int height);
    void onKeyDown(app::Key key, app::ModifierKey modifiers) override;

private:
    SimpleText& parent;
};
