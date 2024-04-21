#pragma once

#include "ui/app/app.h"
#include <vector>

class SimpleText : public App {
public:
    class EditorWindow : public App::Window {
    public:
        int wid;

        EditorWindow(SimpleText& parent, int width, int height, int wid);
        ~EditorWindow();

        void onOpenGLActivate(int width, int height) override;
        void onDraw() override;
        void onResize(int width, int height) override;
        void onKeyDown(app::Key key, app::ModifierKey modifiers) override;
        void onClose() override;

    private:
        SimpleText& parent;
        std::vector<int> memory_waster;
    };

    SimpleText();
    ~SimpleText();
    void createWindow();
    void destroyWindow(int wid);
    void createNWindows(int n);
    void destroyAllWindows();

    void onLaunch() override;

private:
    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};
