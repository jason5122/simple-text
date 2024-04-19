#pragma once

#include "ui/app/app.h"

#include "font/rasterizer.h"

class EditorWindow;

class SimpleText : public App {
public:
    FontRasterizer main_font_rasterizer;
    FontRasterizer ui_font_rasterizer;

    void onLaunch() override;
    void createWindow();
    void destroyWindow(EditorWindow* editor_window);
    void createAllWindows();
    void destroyAllWindows();

private:
    std::vector<EditorWindow*> editor_windows;
};
