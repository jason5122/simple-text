#include "dummy_context.h"

namespace gui {

DummyContext::DummyContext() : window(gtk_window_new(GTK_WINDOW_TOPLEVEL)) {}

void DummyContext::initialize() {}

GdkGLContext* DummyContext::context() {
    GError* error = nullptr;
    GdkGLContext* context =
        gdk_window_create_gl_context(gtk_widget_get_parent_window(window), &error);
    return context;
}

}
