#include "editor_window.h"
#include <epoxy/gl.h>
#include <iostream>

// FIXME: Remove these global variables! Figure out how to use classes with GTK.
RectRenderer* rect_renderer;
ImageRenderer* image_renderer;
TextRenderer* text_renderer;
SyntaxHighlighter highlighter;
Buffer buffer;

double scroll_x = 0;
double scroll_y = 0;

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_q && event->state & GDK_META_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    if (event->keyval == GDK_KEY_q && event->state & GDK_CONTROL_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    return false;
}

static gboolean render(GtkWidget* widget) {
    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;
    double scaled_scroll_x = scroll_x * scale_factor;
    double scaled_scroll_y = scroll_y * scale_factor;
    int scaled_editor_offset_x = 200 * scale_factor;
    int scaled_editor_offset_y = 30 * scale_factor;

    // inside this function it's safe to use GL; the given
    // `GdkGLContext` has been made current to the drawable
    // surface used by the `GtkGLArea` and the viewport has
    // already been set to be the size of the allocation
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer->resize(scaled_width, scaled_height);
    text_renderer->renderText(scaled_scroll_x, scaled_scroll_y, buffer, highlighter,
                              scaled_editor_offset_x, scaled_editor_offset_y);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer->resize(scaled_width, scaled_height);
    rect_renderer->draw(scaled_scroll_x, scaled_scroll_y, 0, 0, 40, 80, 1000,
                        scaled_editor_offset_x, scaled_editor_offset_y);

    image_renderer->resize(scaled_width, scaled_height);
    image_renderer->draw(scaled_scroll_x, scaled_scroll_y);

    // we completed our drawing; the draw commands will be
    // flushed at the end of the signal emission chain, and
    // the buffers will be drawn on the window
    return true;
}

static const char* read(void* payload, uint32_t byte_index, TSPoint position,
                        uint32_t* bytes_read) {
    Buffer* buffer = (Buffer*)payload;
    if (position.row >= buffer->lineCount()) {
        *bytes_read = 0;
        return "";
    }

    const size_t BUFSIZE = 256;
    static char buf[BUFSIZE];

    std::string line_str;
    buffer->getLineContent(&line_str, position.row);

    size_t len = line_str.size();
    size_t bytes_copied = std::min(len - position.column, BUFSIZE);

    memcpy(buf, &line_str[0] + position.column, bytes_copied);
    *bytes_read = (uint32_t)bytes_copied;
    if (bytes_copied < BUFSIZE) {
        // Add the final \n.
        // If it didn't fit, read() will be called again on the same line with the column advanced.
        buf[bytes_copied] = '\n';
        (*bytes_read)++;
    }
    return buf;
}

static void realize(GtkWidget* widget) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget));
    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != nullptr) return;

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;

    int font_size = 16 * scale_factor;
    std::string font_name = "Source Code Pro";

    text_renderer = new TextRenderer();
    rect_renderer = new RectRenderer();
    image_renderer = new ImageRenderer();
    text_renderer->setup(scaled_width, scaled_height, font_name, font_size);
    rect_renderer->setup(scaled_width, scaled_height);
    image_renderer->setup(scaled_width, scaled_height);

    fs::path file_path = ResourcePath() / "sample_files/example.json";
    highlighter.setLanguage("source.json");
    buffer.setContents(ReadFile(file_path));

    TSInput input = {&buffer, read, TSInputEncodingUTF8};
    highlighter.parse(input);
}

static gboolean resize(GtkWidget* widget, GdkEvent* event, gpointer data) {
    std::cerr << "resized\n";
    return false;
}

static gboolean scroll_event(GtkWidget* widget, GdkEventScroll* event, gpointer data) {
    double delta_x, delta_y;
    gdk_event_get_scroll_deltas((GdkEvent*)event, &delta_x, &delta_y);
    std::cerr << delta_x << ", " << delta_y << '\n';

    if (gdk_device_get_source(event->device) == GDK_SOURCE_MOUSE) {
        std::cerr << "mouse\n";
        delta_x *= 32;
        delta_y *= 32;
    }

    scroll_x += delta_x;
    scroll_y += delta_y;

    gtk_widget_queue_draw(widget);

    return true;
}

static void activate(GtkApplication* app) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(window, GDK_CONFIGURE);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), app);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(resize), nullptr);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, false);
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), nullptr);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), nullptr);

    gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), nullptr);

    // FIXME: Dragging a maximized window results in dragging the top left corner.
    //        Using a default window size greater than the screen size seems to maximize without
    //        this issue.
    // gtk_window_maximize(GTK_WINDOW(window));

    GdkDisplay* display = gdk_display_get_default();
    GdkMonitor* monitor = gdk_display_get_monitor(display, 0);
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    gtk_window_set_default_size(GTK_WINDOW(window), geometry.width, geometry.height);

    gtk_widget_show_all(window);
}

EditorWindow::EditorWindow() {
#if GLIB_CHECK_VERSION(2, 74, 0)
    GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS;
#else
    GApplicationFlags flags = G_APPLICATION_FLAGS_NONE;
#endif
    app = gtk_application_new("com.jason.simple-text", flags);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
}

int EditorWindow::run() {
    return g_application_run(G_APPLICATION(app), 0, NULL);
}

EditorWindow::~EditorWindow() {
    g_object_unref(app);
}
