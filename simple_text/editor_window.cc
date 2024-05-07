#include "build/buildflag.h"
#include "gui/key.h"
#include "simple_text.h"
#include <algorithm>
#include <cmath>
#include <glad/glad.h>

using EditorWindow = SimpleText::EditorWindow;

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent, width, height), parent(parent), wid(wid), color_scheme(isDarkMode()) {}

#include <iostream>

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow " << wid << '\n';
}

void EditorWindow::createTab(fs::path file_path) {
    std::unique_ptr<EditorTab> editor_tab = std::make_unique<EditorTab>(file_path);
    editor_tab->setup(color_scheme);
    tabs.insert(tabs.begin() + tab_index, std::move(editor_tab));
}

void EditorWindow::reloadColorScheme() {
    color_scheme.reload(isDarkMode());
    redraw();
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // fs::path file_path = ResourceDir() / "sample_files/example.json";
    fs::path file_path = ResourceDir() / "sample_files/sort.scm";
    fs::path file_path2 = ResourceDir() / "sample_files/proportional_font_test.json";
    fs::path file_path3 = ResourceDir() / "sample_files/worst_case.json";

    createTab(file_path);
    createTab(file_path2);
    createTab(file_path3);
    createTab(fs::path{});

#if IS_LINUX
    // TODO: Implement scale factor support.
    std::string main_font = "Source Code Pro";
    std::string ui_font = "Noto Sans";
    int main_font_size = 16 * 2;
    int ui_font_size = 11 * 2;

    main_font_rasterizer.setup(0, main_font, main_font_size);
    ui_font_rasterizer.setup(1, ui_font, ui_font_size);

    text_renderer.setup(main_font_rasterizer);
    rect_renderer.setup();
    image_renderer.setup();
    selection_renderer.setup(main_font_rasterizer);
#endif
}

void EditorWindow::onDraw() {
    using Selection = renderer::SelectionRenderer::Selection;

    renderer::Size size{
        .width = width() * scaleFactor(),
        .height = height() * scaleFactor(),
    };

    glViewport(0, 0, size.width, size.height);

    Rgb& background = color_scheme.background;
    GLfloat red = background.r / 255.0;
    GLfloat green = background.g / 255.0;
    GLfloat blue = background.b / 255.0;
    glClearColor(red, green, blue, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    FontRasterizer& ui_font_rasterizer = parent.ui_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
    renderer::RectRenderer& rect_renderer = parent.rect_renderer;
    renderer::ImageRenderer& image_renderer = parent.image_renderer;
    renderer::SelectionRenderer& selection_renderer = parent.selection_renderer;
#endif

    int status_bar_height = ui_font_rasterizer.line_height;

    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    // Setup.
    std::vector<Selection> selections = text_renderer.getSelections(
        tab->buffer, main_font_rasterizer, tab->start_caret, tab->end_caret);

    selection_renderer.createInstances(size, tab->scroll, editor_offset, main_font_rasterizer,
                                       selections, kLineNumberOffset);

    // Render.
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    selection_renderer.render(0);
    text_renderer.renderText(size, tab->scroll, tab->buffer, tab->highlighter, editor_offset,
                             main_font_rasterizer, status_bar_height, tab->start_caret,
                             tab->end_caret, tab->longest_line_x, color_scheme, kLineNumberOffset);
    selection_renderer.render(1);

    std::vector<int> tab_title_widths =
        text_renderer.getTabTitleWidths(tab->buffer, ui_font_rasterizer, tabs);
    std::vector<int> tab_title_x_coords;
    std::vector<int> actual_tab_title_widths;

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
    rect_renderer.draw(size, tab->scroll, tab->end_caret, main_font_rasterizer.line_height,
                       tab->buffer.lineCount(), tab->longest_line_x, editor_offset,
                       status_bar_height, color_scheme, tab_index, tab_title_widths,
                       kLineNumberOffset, tab_title_x_coords, actual_tab_title_widths);

    image_renderer.draw(size, tab->scroll, editor_offset, tab_title_x_coords,
                        actual_tab_title_widths);

    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    text_renderer.renderUiText(size, main_font_rasterizer, ui_font_rasterizer, tab->end_caret,
                               color_scheme, editor_offset, tabs, tab_title_x_coords);

    // Cleanup.
    selection_renderer.destroyInstances();
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(float dx, float dy) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    // TODO: Stopping at fractional pixels causes glitchy artifacts, so we round to prevent this.
    // However, this stops scrolling from feeling natural. This is an issue for GTK.
    // Consider changing onScroll() signature to only accept integers.
    dx = std::round(dx);
    dy = std::round(dy);

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
#endif

    float buffer_width = width() * scaleFactor() - editor_offset.x;
    float max_scroll_x = std::max(0.0f, tab->longest_line_x - buffer_width);
    // TODO: Subtract one from line count to leave the last line visible.
    float max_scroll_y = tab->buffer.lineCount() * main_font_rasterizer.line_height;

    tab->scrollBuffer({dx, dy}, {max_scroll_x, max_scroll_y});

    redraw();
}

void EditorWindow::onLeftMouseDown(float mouse_x, float mouse_y) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point mouse{
        .x = mouse_x - editor_offset.x - kLineNumberOffset + tab->scroll.x,
        .y = mouse_y - editor_offset.y + tab->scroll.y,
    };

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

    text_renderer.setCaretInfo(tab->buffer, main_font_rasterizer, mouse, tab->start_caret);
    tab->end_caret = tab->start_caret;

    redraw();
}

void EditorWindow::onLeftMouseDrag(float mouse_x, float mouse_y) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point mouse{
        .x = mouse_x - editor_offset.x - kLineNumberOffset + tab->scroll.x,
        .y = mouse_y - editor_offset.y + tab->scroll.y,
    };

#if IS_MAC || IS_WIN
    FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
    renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

    text_renderer.setCaretInfo(tab->buffer, main_font_rasterizer, mouse, tab->end_caret);

    redraw();
}

static inline int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createWindow();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        close();
    }

    // if (key == app::Key::kA && modifiers == app::kPrimaryModifier) {
    //     parent.createNWindows(25);
    // }
    // if (key == app::Key::kB && modifiers == app::kPrimaryModifier) {
    //     parent.destroyAllWindows();
    // }

    // if (key == app::Key::kC && modifiers == app::kPrimaryModifier) {
    //     close();
    // }

    if (key == app::Key::kJ && modifiers == app::kPrimaryModifier) {
        tab_index = positive_modulo(tab_index - 1, tabs.size());
        redraw();
    }
    if (key == app::Key::kK && modifiers == app::kPrimaryModifier) {
        tab_index = positive_modulo(tab_index + 1, tabs.size());
        redraw();
    }
    if (key == app::Key::kW && modifiers == app::kPrimaryModifier) {
        tabs.erase(tabs.begin() + tab_index);
        tab_index--;
        if (tab_index < 0) {
            tab_index = 0;
        }

        if (tabs.empty()) {
            createTab(fs::path{});
        }

        redraw();
    }

    if (key == app::Key::kN && modifiers == app::kPrimaryModifier) {
        createTab(fs::path{});
        redraw();
    }

    if (key == app::Key::k1 && modifiers == app::kPrimaryModifier) {
        tab_index = 0;
        redraw();
    }
    if (key == app::Key::k2 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 1) {
            tab_index = 1;
        }
        redraw();
    }
    if (key == app::Key::k3 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 2) {
            tab_index = 2;
        }
        redraw();
    }
    if (key == app::Key::k4 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 3) {
            tab_index = 3;
        }
        redraw();
    }
    if (key == app::Key::k5 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 4) {
            tab_index = 4;
        }
        redraw();
    }
    if (key == app::Key::k6 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 5) {
            tab_index = 5;
        }
        redraw();
    }
    if (key == app::Key::k7 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 6) {
            tab_index = 6;
        }
        redraw();
    }
    if (key == app::Key::k8 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 7) {
            tab_index = 7;
        }
        redraw();
    }
    if (key == app::Key::k9 && modifiers == app::kPrimaryModifier) {
        if (tabs.size() > 0) {
            tab_index = tabs.size() - 1;
        }
        redraw();
    }

    if (key == app::Key::k0 && modifiers == app::kPrimaryModifier) {
        if (side_bar_visible) {
            editor_offset.x = 0;
        } else {
            editor_offset.x = 200 * 2;
        }
        side_bar_visible = !side_bar_visible;
        redraw();
    }

    if (key == app::Key::kBackspace && modifiers == app::ModifierKey::kNone) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
        size_t start_byte = tab->start_caret.byte, end_byte = tab->end_caret.byte;
        if (tab->start_caret.byte > tab->end_caret.byte) {
            std::swap(start_byte, end_byte);
        }

        tab->buffer.erase(start_byte, end_byte);

        // TODO: Move this into EditorTab.
        tab->highlighter.edit(start_byte, end_byte, start_byte);
        TSInput input = {&tab->buffer, Buffer::read, TSInputEncodingUTF8};
        tab->highlighter.parse(input);

        if (tab->start_caret.byte > tab->end_caret.byte) {
            tab->start_caret = tab->end_caret;
        } else {
            tab->end_caret = tab->start_caret;
        }

        redraw();
    }

    if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kNone) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
#if IS_MAC || IS_WIN
        FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
        renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

        text_renderer.moveCaretForwardChar(tab->buffer, tab->end_caret, main_font_rasterizer);
        redraw();
    }
    if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kAlt) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
#if IS_MAC || IS_WIN
        FontRasterizer& main_font_rasterizer = parent.main_font_rasterizer;
        renderer::TextRenderer& text_renderer = parent.text_renderer;
#endif

        text_renderer.moveCaretForwardWord(tab->buffer, tab->end_caret, main_font_rasterizer);
        redraw();
    }

    if (key == app::Key::kQ && modifiers == app::kPrimaryModifier) {
        parent.quit();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::onDarkModeToggle() {
    reloadColorScheme();
}
