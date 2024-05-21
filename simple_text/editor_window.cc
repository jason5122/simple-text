#include "build/buildflag.h"
#include "simple_text.h"
#include "util/profile_util.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <glad/glad.h>
#include <iostream>

using EditorWindow = SimpleText::EditorWindow;

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent, width, height), parent(parent), wid(wid), color_scheme(isDarkMode()),
      main_font_rasterizer(parent.main_font_rasterizer),
      ui_font_rasterizer(parent.ui_font_rasterizer)
#if IS_MAC || IS_WIN
      ,
      renderer(parent.renderer)
#endif
{
}

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

#if IS_LINUX
    main_glyph_cache.setup();
    ui_glyph_cache.setup();
    renderer.setup();

    text_renderer.setup();
    rect_renderer.setup();
    image_renderer.setup();
    selection_renderer.setup();
#endif

    fs::path file_path = ResourceDir() / "sample_files/sort.scm";
    fs::path file_path2 = ResourceDir() / "sample_files/proportional_font_test.json";
    fs::path file_path3 = ResourceDir() / "sample_files/worst_case.json";
    fs::path file_path4 = ResourceDir() / "sample_files/example.json";
    fs::path file_path5 = ResourceDir() / "sample_files/long_lines.json";

    createTab(file_path);
    createTab(file_path2);
    createTab(file_path3);
    createTab(file_path4);
    createTab(file_path5);
}

void EditorWindow::onDraw() {
    renderer::Size size{
        .width = width() * scaleFactor(),
        .height = height() * scaleFactor(),
    };
    renderer.render(size, color_scheme, tabs, tab_index);
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    int buffer_width = width() * scaleFactor() - editor_offset.x;
    int max_scroll_x = std::max(0, tab->longest_line_x - buffer_width);
    // TODO: Subtract one from line count to leave the last line visible.
    int max_scroll_y = tab->buffer.lineCount() * main_font_rasterizer.line_height;

    renderer::Point delta{dx, dy};
    renderer::Point max_scroll{max_scroll_x, max_scroll_y};
    tab->scrollBuffer(delta, max_scroll);

    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point mouse{
        .x = mouse_x - editor_offset.x - kLineNumberOffset + tab->scroll.x,
        .y = mouse_y - editor_offset.y + tab->scroll.y,
    };

    renderer.text_renderer.setCaretInfo(tab->buffer, mouse, tab->end_caret);
    if (!Any(modifiers & app::ModifierKey::kShift)) {
        tab->start_caret = tab->end_caret;
    }

    redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point mouse{
        .x = mouse_x - editor_offset.x - kLineNumberOffset + tab->scroll.x,
        .y = mouse_y - editor_offset.y + tab->scroll.y,
    };

    renderer.text_renderer.setCaretInfo(tab->buffer, mouse, tab->end_caret);

    redraw();
}

static inline int PositiveModulo(int i, int n) {
    return (i % n + n) % n;
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    using config::Action;

    // Action action = parent.key_bindings.parseKeyPress(key, modifiers);
    Action action;
    {
        PROFILE_BLOCK("KeyBindings::parseKeyPress()");
        action = parent.key_bindings.parseKeyPress(key, modifiers);
    }

    switch (action) {
    case Action::kNone:
        break;

    case Action::kNewWindow:
        parent.createWindow();
        break;

    case Action::kCloseWindow:
        close();
        break;

    case Action::kNewTab:
        createTab(fs::path{});
        redraw();
        break;

    case Action::kCloseTab:
        tabs.erase(tabs.begin() + tab_index);
        tab_index--;
        if (tab_index < 0) {
            tab_index = 0;
        }

        if (tabs.empty()) {
            // createTab(fs::path{});
            close();
        } else {
            redraw();
        }
        break;

    case Action::kPreviousTab:
        tab_index = PositiveModulo(tab_index - 1, tabs.size());
        redraw();
        break;

    case Action::kNextTab:
        tab_index = PositiveModulo(tab_index + 1, tabs.size());
        redraw();
        break;

    case Action::kSelectTab1:
    case Action::kSelectTab2:
    case Action::kSelectTab3:
    case Action::kSelectTab4:
    case Action::kSelectTab5:
    case Action::kSelectTab6:
    case Action::kSelectTab7:
    case Action::kSelectTab8: {
        using U = std::underlying_type_t<app::Key>;
        int index = static_cast<U>(key) - static_cast<U>(app::Key::k1);

        if (tabs.size() > index) {
            tab_index = index;
        }
        redraw();
        break;
    }

    case Action::kSelectLastTab:
        if (tabs.size() > 0) {
            tab_index = tabs.size() - 1;
        }
        redraw();
        break;

    case Action::kToggleSideBar:
        if (side_bar_visible) {
            editor_offset.x = 0;
        } else {
            editor_offset.x = 200 * 2;
        }
        side_bar_visible = !side_bar_visible;
        redraw();
        break;
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

    if (key == app::Key::kBackspace && modifiers == app::ModifierKey::kNone) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
        size_t start_byte = tab->start_caret.byte, end_byte = tab->end_caret.byte;
        if (tab->start_caret.byte > tab->end_caret.byte) {
            std::swap(start_byte, end_byte);
        }

        tab->buffer.erase(start_byte, end_byte);

        // TODO: Move this into EditorTab.
        tab->highlighter.edit(start_byte, end_byte, start_byte);
        TSInput input = {&tab->buffer, SyntaxHighlighter::read, TSInputEncodingUTF8};
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
        renderer.text_renderer.moveCaretForwardChar(tab->buffer, tab->end_caret);
        redraw();
    }
    if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kAlt) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
        renderer.text_renderer.moveCaretForwardWord(tab->buffer, tab->end_caret);
        redraw();
    }

    if (key == app::Key::kQ && modifiers == app::kPrimaryModifier) {
        parent.quit();
    }

    if (app::Key::kA <= key && key <= app::Key::kZ && modifiers == app::ModifierKey::kNone) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

        using U = std::underlying_type_t<app::Key>;
        char ch = static_cast<char>('a' + (static_cast<U>(key) - static_cast<U>(app::Key::kA)));

        tab->buffer.insert(tab->end_caret.line, tab->end_caret.column, std::string(1, ch));
        redraw();
    }
    if (app::Key::kA <= key && key <= app::Key::kZ && modifiers == app::ModifierKey::kShift) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

        using U = std::underlying_type_t<app::Key>;
        char ch = static_cast<char>('a' + (static_cast<U>(key) - static_cast<U>(app::Key::kA)));
        ch = static_cast<char>(std::toupper(ch));

        tab->buffer.insert(tab->end_caret.line, tab->end_caret.column, std::string(1, ch));
        redraw();
    }
    if (key == app::Key::kEnter && modifiers == app::ModifierKey::kNone) {
        std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
        tab->buffer.insert(tab->end_caret.line, tab->end_caret.column, "\n");
        redraw();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::onDarkModeToggle() {
    reloadColorScheme();
}
