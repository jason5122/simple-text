#pragma once

#include <gtk/gtk.h>

namespace gui {

class DummyContext {
public:
    DummyContext();
    // TODO: See if we can move initialization code into the constructor.
    void initialize();
    GdkGLContext* context();

private:
    GtkWidget* window;
};

}
