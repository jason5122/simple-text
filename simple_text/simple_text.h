#pragma once

#include "ui/app/app.h"
#include "ui/app/cocoa/displaygl.h"

#include "font/rasterizer.h"

class EditorWindow;

class SimpleText : public App {
public:
    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;

    void onLaunch() override;
    void createChild();
    void destroyChild(EditorWindow* editor_window);

private:
    DisplayGL displaygl;
};
