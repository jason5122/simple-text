#include "base/buffer.h"
#import "base/syntax_highlighter.h"
#include "editor_window.h"
#import "font/rasterizer.h"
#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/renderer/text_renderer.h"
#include <epoxy/gl.h>
#include <iostream>

// FIXME: Remove these global variables! Figure out how to use classes with GTK.
RectRenderer* rect_renderer;
ImageRenderer* image_renderer;
TextRenderer* text_renderer;
SyntaxHighlighter highlighter;
Buffer buffer;
FontRasterizer main_font_rasterizer;
FontRasterizer ui_font_rasterizer;

double scroll_x = 0;
double scroll_y = 0;
double cursor_start_x = 0;
double cursor_start_y = 0;
double cursor_end_x = 0;
double cursor_end_y = 0;
float editor_offset_x = 200;
float editor_offset_y = 30;

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

static gboolean my_keypress_function(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_q && event->state & GDK_META_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }
    if (event->keyval == GDK_KEY_q && event->state & GDK_CONTROL_MASK) {
        g_application_quit(G_APPLICATION(data));
        return true;
    }

    // Ignore modifier-only key presses.
    if (event->is_modifier) {
        return false;
    }

    const gchar* str;
    size_t bytes = 1;
    if (event->keyval == GDK_KEY_Return) {
        str = "\n";
    } else {
        gunichar unicode = gdk_keyval_to_unicode(event->keyval);
        str = g_ucs4_to_utf8(&unicode, bytes, nullptr, nullptr, nullptr);
        std::cerr << "str: " << str << " keyval: " << event->keyval << '\n';
    }

    buffer.insert(text_renderer->cursor_end_line, text_renderer->cursor_end_col_offset, str);

    // TODO: Move this into an edit_buffer() method.
    size_t start_byte =
        buffer.byteOfLine(text_renderer->cursor_end_line) + text_renderer->cursor_end_col_offset;
    size_t old_end_byte =
        buffer.byteOfLine(text_renderer->cursor_end_line) + text_renderer->cursor_end_col_offset;
    size_t new_end_byte = buffer.byteOfLine(text_renderer->cursor_end_line) +
                          text_renderer->cursor_end_col_offset + bytes;
    highlighter.edit(start_byte, old_end_byte, new_end_byte);

    // TODO: Move this into a parse_buffer() method.
    TSInput input = {&buffer, read, TSInputEncodingUTF8};
    highlighter.parse(input);

    // FIXME: Do this under an OpenGL context!
    //        Without that step, glyphs are not loaded into the atlas correctly.
    // if (strcmp(str, "\n") == 0) {
    //     text_renderer->cursor_start_line++;
    //     text_renderer->cursor_end_line++;

    //     text_renderer->cursor_start_col_offset = 0;
    //     text_renderer->cursor_start_x = 0;
    //     text_renderer->cursor_end_col_offset = 0;
    //     text_renderer->cursor_end_x = 0;
    // } else {
    //     float advance = text_renderer->getGlyphAdvance(std::string(str));
    //     text_renderer->cursor_start_col_offset += bytes;
    //     text_renderer->cursor_start_x += advance;
    //     text_renderer->cursor_end_col_offset += bytes;
    //     text_renderer->cursor_end_x += advance;
    // }

    gtk_widget_queue_draw(widget);

    return false;
}

static gboolean render(GtkWidget* widget) {
    int scale_factor = gtk_widget_get_scale_factor(widget);
    int scaled_width = gtk_widget_get_allocated_width(widget) * scale_factor;
    int scaled_height = gtk_widget_get_allocated_height(widget) * scale_factor;
    double scaled_scroll_x = scroll_x * scale_factor;
    double scaled_scroll_y = scroll_y * scale_factor;
    int scaled_editor_offset_x = editor_offset_x * scale_factor;
    int scaled_editor_offset_y = editor_offset_y * scale_factor;
    float scaled_status_bar_height = ui_font_rasterizer.line_height;

    std::cerr << "raw dimensions: " << gtk_widget_get_allocated_width(widget) << "x"
              << gtk_widget_get_allocated_height(widget) << '\n';

    // inside this function it's safe to use GL; the given
    // `GdkGLContext` has been made current to the drawable
    // surface used by the `GtkGLArea` and the viewport has
    // already been set to be the size of the allocation
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer->resize(scaled_width, scaled_height);
    text_renderer->renderText(scaled_scroll_x, scaled_scroll_y, buffer, highlighter,
                              scaled_editor_offset_x, scaled_editor_offset_y, main_font_rasterizer,
                              scaled_status_bar_height);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer->resize(scaled_width, scaled_height);
    rect_renderer->draw(scaled_scroll_x, scaled_scroll_y, text_renderer->cursor_end_x,
                        text_renderer->cursor_end_line, main_font_rasterizer.line_height,
                        buffer.lineCount(), text_renderer->longest_line_x, scaled_editor_offset_x,
                        scaled_editor_offset_y, scaled_status_bar_height);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    image_renderer->resize(scaled_width, scaled_height);
    image_renderer->draw(scaled_scroll_x, scaled_scroll_y, scaled_editor_offset_x,
                         scaled_editor_offset_y);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer->renderUiText(main_font_rasterizer, ui_font_rasterizer);

    // we completed our drawing; the draw commands will be
    // flushed at the end of the signal emission chain, and
    // the buffers will be drawn on the window
    return true;
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
    std::string font_name = "SourceCodePro-Regular.ttf";

    text_renderer = new TextRenderer();
    rect_renderer = new RectRenderer();
    image_renderer = new ImageRenderer();
    main_font_rasterizer.setup(0, "SourceCodePro-Regular.ttf", 16 * scale_factor);
    ui_font_rasterizer.setup(1, "SourceCodePro-Regular.ttf", 11 * scale_factor);
    text_renderer->setup(scaled_width, scaled_height, main_font_rasterizer);
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
    if (scroll_x < 0) scroll_x = 0;
    if (scroll_y < 0) scroll_y = 0;

    gtk_widget_queue_draw(widget);

    return true;
}

// https://github.com/ToshioCP/Gtk4-tutorial/blob/main/gfm/sec17.md#menu-and-action
static void quit_callback(GSimpleAction* action, GVariant* parameter, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

static gboolean button_event(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    if (event->type == GDK_BUTTON_PRESS) {
        std::cerr << "press\n";

        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        mouse_x -= editor_offset_x;
        mouse_y -= editor_offset_y;

        mouse_x += scroll_x;
        mouse_y += scroll_y;

        cursor_start_x = mouse_x;
        cursor_start_y = mouse_y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        text_renderer->setCursorPositions(buffer, cursor_start_x * scale_factor,
                                          cursor_start_y * scale_factor, mouse_x * scale_factor,
                                          mouse_y * scale_factor, main_font_rasterizer);

        gtk_widget_queue_draw(widget);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        std::cerr << "release\n";
    }
    return true;
}

// https://stackoverflow.com/a/43721112
static gboolean on_crossing(GtkWidget* widget, GdkEventCrossing* event) {
    GdkDisplay* display = gtk_widget_get_display(widget);
    GdkCursor* cursor;
    if (event->type == GDK_ENTER_NOTIFY) {
        cursor = gdk_cursor_new_from_name(display, "text");
    }
    if (event->type == GDK_LEAVE_NOTIFY) {
        cursor = gdk_cursor_new_from_name(display, "default");
    }
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    g_object_unref(cursor);
    return true;
}

static gboolean motion_event(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    if (event->type == GDK_MOTION_NOTIFY) {
        std::cerr << "GDK_MOTION_NOTIFY\n";

        gdouble mouse_x = event->x;
        gdouble mouse_y = event->y;

        mouse_x -= editor_offset_x;
        mouse_y -= editor_offset_y;

        mouse_x += scroll_x;
        mouse_y += scroll_y;

        int scale_factor = gtk_widget_get_scale_factor(widget);
        text_renderer->setCursorPositions(buffer, cursor_start_x * scale_factor,
                                          cursor_start_y * scale_factor, mouse_x * scale_factor,
                                          mouse_y * scale_factor, main_font_rasterizer);

        gtk_widget_queue_draw(widget);
    }
    return true;
}

static void activate(GtkApplication* app) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Text");

    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), app);
    gtk_widget_add_events(window, GDK_CONFIGURE);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(resize), nullptr);

    GMenu* menu_bar = g_menu_new();
    GMenu* file_menu = g_menu_new();
    GMenuItem* quit_menu_item = g_menu_item_new("Quit", "app.quit");

    g_menu_append_submenu(menu_bar, "File", G_MENU_MODEL(file_menu));
    g_menu_append_item(file_menu, quit_menu_item);
    gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(menu_bar));
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);

    GSimpleAction* quit_action = g_simple_action_new("quit", nullptr);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit_action));
    g_signal_connect(quit_action, "activate", G_CALLBACK(quit_callback), app);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, false);
    gtk_box_set_spacing(GTK_BOX(box), 6);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_box_pack_start(GTK_BOX(box), gl_area, 1, 1, 0);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), nullptr);
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), nullptr);

    gtk_widget_add_events(gl_area, GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(gl_area, "scroll-event", G_CALLBACK(scroll_event), nullptr);

    gtk_widget_add_events(gl_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(gl_area), "button-press-event", G_CALLBACK(button_event), nullptr);
    g_signal_connect(G_OBJECT(gl_area), "button-release-event", G_CALLBACK(button_event), nullptr);
    gtk_widget_add_events(gl_area, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(G_OBJECT(gl_area), "enter-notify-event", G_CALLBACK(on_crossing), nullptr);
    g_signal_connect(G_OBJECT(gl_area), "leave-notify-event", G_CALLBACK(on_crossing), nullptr);

    gtk_widget_add_events(gl_area, GDK_BUTTON1_MOTION_MASK);
    g_signal_connect(G_OBJECT(gl_area), "motion-notify-event", G_CALLBACK(motion_event), nullptr);

    gtk_window_maximize(GTK_WINDOW(window));
    // TODO: Set default window size without magic numbers.
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);

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
