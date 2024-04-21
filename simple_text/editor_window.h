#pragma once

#include "simple_text/simple_text.h"
#include "ui/app/app.h"

class EditorWindow : public App::Window {
public:
    EditorWindow(SimpleText& parent, int width, int height);
    void customDelete();

    void onOpenGLActivate(int width, int height) override;
    void onDraw() override;
    void onResize(int width, int height) override;
    void onKeyDown(app::Key key, app::ModifierKey modifiers) override;
    void onClose() override;

private:
    SimpleText& parent;
};
