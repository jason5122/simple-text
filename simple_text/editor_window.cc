#include "build/buildflag.h"
#include "editor_window.h"
#include "simple_text/simple_text.h"

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode())
#if IS_MAC || IS_WIN
      ,
      renderer(parent.renderer)
#elif IS_LINUX
      ,
      renderer(parent.gl)
#endif
{
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    renderer.setup();
}

void EditorWindow::onDraw() {
    renderer.draw();
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
