#include "build/buildflag.h"
#include "simple_text.h"
#include "simple_text/action_executer.h"
#include <algorithm>
#include <cctype>
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
#elif IS_LINUX
      ,
      renderer(main_font_rasterizer, ui_font_rasterizer)
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
    redraw();
}

void EditorWindow::reloadColorScheme() {
    color_scheme.reload(isDarkMode());
    redraw();
}

void EditorWindow::selectTabIndex(int index) {
    if (tabs.size() > index) {
        tab_index = index;
    }
    redraw();
}

static inline int PositiveModulo(int i, int n) {
    return (i % n + n) % n;
}

void EditorWindow::selectPreviousTab() {
    tab_index = PositiveModulo(tab_index - 1, tabs.size());
    redraw();
}

void EditorWindow::selectNextTab() {
    tab_index = PositiveModulo(tab_index + 1, tabs.size());
    redraw();
}

void EditorWindow::selectLastTab() {
    if (tabs.size() > 0) {
        tab_index = tabs.size() - 1;
    }
    redraw();
}

void EditorWindow::closeCurrentTab() {
    tabs.erase(tabs.begin() + tab_index);
    tab_index--;
    if (tab_index < 0) {
        tab_index = 0;
    }

    if (tabs.empty()) {
        close();
    } else {
        redraw();
    }
}

void EditorWindow::toggleSideBar() {
    // renderer.toggleSideBar();
    redraw();
}

void EditorWindow::onOpenGLActivate(int width, int height) {
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

#if IS_LINUX
    renderer.setup();
#endif

    fs::path file_path = ResourceDir() / "sample_files/sort.scm";
    fs::path file_path2 = ResourceDir() / "sample_files/proportional_font_test.json";
    fs::path file_path3 = ResourceDir() / "sample_files/worst_case.json";
    fs::path file_path4 = ResourceDir() / "sample_files/example.json";
    fs::path file_path5 = ResourceDir() / "sample_files/long_lines.json";
    fs::path file_path6 = ResourceDir() / "sample_files/emoji-data.txt";
    fs::path file_path7 = ResourceDir() / "sample_files/emoji-test.txt";

    // createTab(file_path);
    // createTab(file_path2);
    // createTab(file_path3);
    // createTab(file_path4);
    // createTab(file_path5);
    // createTab(file_path6);
    // createTab(file_path7);
    createTab({});
}

void EditorWindow::onDraw() {
    renderer::Size size{
        .width = width() * scaleFactor(),
        .height = height() * scaleFactor(),
    };
    renderer.render(size, color_scheme, tabs, tab_index);

    // TODO: For debugging; remove this.
    if (!has_drawn) {
        has_drawn = true;

        auto draw_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(draw_time - parent.launch_time)
                .count();
        std::cerr << std::format("startup time: {} µs", duration) << '\n';
    }
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    // TODO: Move to Renderer and incorporate `editor_offset.x`.
    int buffer_width = width() * scaleFactor();
    // int buffer_width = width() * scaleFactor() - editor_offset.x;

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
    // renderer.setCaretPosition(mouse_x, mouse_y, tab);

    if (!Any(modifiers & app::ModifierKey::kShift)) {
        tab->start_caret = tab->end_caret;
    }

    redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    // renderer.setCaretPosition(mouse_x, mouse_y, tab);

    redraw();
}

void EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    using config::Action;

    Action action = parent.key_bindings.parseKeyPress(key, modifiers);
    ExecuteAction(action, parent, *this);

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
        // tab->highlighter.edit(start_byte, end_byte, start_byte);
        // TSInput input = {&tab->buffer, SyntaxHighlighter::read, TSInputEncodingUTF8};
        // tab->highlighter.parse(input);

        if (tab->start_caret.byte > tab->end_caret.byte) {
            tab->start_caret = tab->end_caret;
        } else {
            tab->end_caret = tab->start_caret;
        }

        redraw();
    }

    // if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kNone) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretForwardChar(tab->buffer, tab->end_caret);
    //     tab->start_caret = tab->end_caret;
    //     redraw();
    // }
    // if (key == app::Key::kLeftArrow && modifiers == app::ModifierKey::kNone) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretBackwardChar(tab->buffer, tab->end_caret);
    //     tab->start_caret = tab->end_caret;
    //     redraw();
    // }
    // if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kShift) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretForwardChar(tab->buffer, tab->end_caret);
    //     redraw();
    // }
    // if (key == app::Key::kLeftArrow && modifiers == app::ModifierKey::kShift) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretBackwardChar(tab->buffer, tab->end_caret);
    //     redraw();
    // }
    // if (key == app::Key::kRightArrow && modifiers == app::ModifierKey::kAlt) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretForwardWord(tab->buffer, tab->end_caret);
    //     tab->start_caret = tab->end_caret;
    //     redraw();
    // }
    // if (key == app::Key::kLeftArrow && modifiers == app::ModifierKey::kAlt) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretBackwardWord(tab->buffer, tab->end_caret);
    //     tab->start_caret = tab->end_caret;
    //     redraw();
    // }
    // if (key == app::Key::kRightArrow &&
    //     modifiers == (app::ModifierKey::kShift | app::ModifierKey::kAlt)) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretForwardWord(tab->buffer, tab->end_caret);
    //     redraw();
    // }
    // if (key == app::Key::kLeftArrow &&
    //     modifiers == (app::ModifierKey::kShift | app::ModifierKey::kAlt)) {
    //     std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    //     renderer.movement.moveCaretBackwardWord(tab->buffer, tab->end_caret);
    //     redraw();
    // }

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
