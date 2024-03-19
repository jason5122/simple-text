#pragma once

#include "base/fredbuf_buffer.h"
#import "base/syntax_highlighter.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include <gtk/gtk.h>

class EditorWindow {
public:
    EditorWindow();
    ~EditorWindow();
    int run();

private:
    GtkApplication* app;
};
