#pragma once

#include "ui/app/app.h"

class EditorWindow;

class SimpleText : public Parent {
public:
    void onLaunch() override;
    void createChild();
    void destroyChild(EditorWindow* editor_window);
};
