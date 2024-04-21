#pragma once

#include "ui/app/app.h"
#include <vector>

// class EditorWindow;

class SimpleText : public App {
public:
    class EditorWindow : public App::Window {
    public:
        EditorWindow(SimpleText& parent, int width, int height);
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

    SimpleText() {}
    ~SimpleText() {}

    void onLaunch() override;
    void createWindow();
    void destroyWindow(EditorWindow* editor_window);
    void createAllWindows();
    void destroyAllWindows();

private:
    std::vector<EditorWindow*> editor_windows;
    std::vector<std::unique_ptr<EditorWindow>> editor_windows_unique;
    int window_count = 0;
};
