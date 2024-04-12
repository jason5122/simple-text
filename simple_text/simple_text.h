#pragma once

#include "ui/app/app.h"

class EditorWindow;

class SimpleText : public App {
public:
    void onLaunch() override;
    void createChild(int x, int y);
    void destroyChild(EditorWindow* editor_window);
};
