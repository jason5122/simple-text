#pragma once

#include "ui/app/app.h"
#include "ui/app/cocoa/displaygl.h"

#include "font/rasterizer.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"

class EditorWindow;

class SimpleText : public App {
public:
    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;

    TextRenderer text_renderer;
    RectRenderer rect_renderer;
    ImageRenderer image_renderer;

    void onLaunch() override;
    void createChild();
    void destroyChild(EditorWindow* editor_window);

private:
    DisplayGL displaygl;
};
