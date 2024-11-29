#pragma once

#include "app/window.h"

class EditorApp;

class EditorWindow : public app::Window {
public:
    EditorWindow(EditorApp& parent, int width, int height, int wid);

    void onDraw(const app::Size& size) override;
};
