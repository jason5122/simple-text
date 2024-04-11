#pragma once

#include "base/buffer.h"
#include "base/syntax_highlighter.h"
#include "font/rasterizer.h"
#include "ui/app/app.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"

class EditorWindow : public App::Window {
public:
    EditorWindow(App& app, int idx)
        : App::Window(app), idx(idx), temp(new std::vector(100000000, 1)) {}
    ~EditorWindow();

private:
    int idx;
    std::vector<int>* temp;

    void onKeyDownVirtual(app::Key key, app::ModifierKey modifiers);
};
