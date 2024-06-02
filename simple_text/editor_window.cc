#include "build/buildflag.h"
#include "editor_window.h"
#include "simple_text/action_executer.h"
#include <algorithm>
#include <cctype>
#include <glad/glad.h>
#include <iostream>

EditorWindow::EditorWindow(SimpleText& parent, int width, int height, int wid)
    : Window(parent), parent(parent), wid(wid), color_scheme(isDarkMode())
#if IS_MAC || IS_WIN
      ,
      renderer(parent.renderer)
#elif IS_LINUX
      ,
      renderer(parent.main_font_rasterizer, parent.ui_font_rasterizer)
#endif
{
}

EditorWindow::~EditorWindow() {
    std::cerr << "~EditorWindow " << wid << '\n';
}

void EditorWindow::createTab(fs::path file_path) {
    std::unique_ptr<EditorTab> editor_tab =
        std::make_unique<EditorTab>(file_path, renderer.movement);
    editor_tab->setup(color_scheme);
    tabs.insert(tabs.begin() + tab_index, std::move(editor_tab));
    updateWindowTitle();
}

void EditorWindow::reloadColorScheme() {
    color_scheme.reload(isDarkMode());
    redraw();
}

void EditorWindow::selectTabIndex(int index) {
    if (tabs.size() > index) {
        tab_index = index;
        updateWindowTitle();
    }
    redraw();
}

static inline int PositiveModulo(int i, int n) {
    return (i % n + n) % n;
}

void EditorWindow::selectPreviousTab() {
    tab_index = PositiveModulo(tab_index - 1, tabs.size());
    updateWindowTitle();
    redraw();
}

void EditorWindow::selectNextTab() {
    tab_index = PositiveModulo(tab_index + 1, tabs.size());
    updateWindowTitle();
    redraw();
}

void EditorWindow::selectLastTab() {
    if (tabs.size() > 0) {
        tab_index = tabs.size() - 1;
        updateWindowTitle();
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
        updateWindowTitle();
        redraw();
    }
}

void EditorWindow::toggleSideBar() {
    renderer.toggleSideBar();
    redraw();
}

void EditorWindow::updateWindowTitle() {
    setTitle(tabs[tab_index]->file_path.filename());
    setFilePath(tabs[tab_index]->file_path);
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

    createTab(file_path);
    createTab(file_path2);
    createTab(file_path3);
    createTab(file_path4);
    createTab(file_path5);
    createTab(file_path6);
    createTab(file_path7);
    createTab({});
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

    // TODO: Move to Renderer and incorporate `editor_offset.x`.
    int buffer_width = width() * scaleFactor();
    // int buffer_width = width() * scaleFactor() - editor_offset.x;

    int max_scroll_x = std::max(0, tab->longest_line_x - buffer_width);
    // TODO: Subtract one from line count to leave the last line visible.
    int max_scroll_y = tab->buffer.lineCount() * renderer.lineHeight();

    renderer::Point delta{dx, dy};
    renderer::Point max_scroll{max_scroll_x, max_scroll_y};
    tab->scrollBuffer(delta, max_scroll);

    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point pos = renderer.translateMousePosition(mouse_x, mouse_y, tab.get());

    bool extend = Any(modifiers & gui::ModifierKey::kShift);
    tab->setCaretPosition(pos, extend);

    redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);

    renderer::Point pos = renderer.translateMousePosition(mouse_x, mouse_y, tab.get());
    tab->setCaretPosition(pos, true);

    redraw();
}

bool EditorWindow::onKeyDown(gui::Key key, gui::ModifierKey modifiers) {
    using config::Action;

    Action action = parent.key_bindings.parseKeyPress(key, modifiers);

    if (action == Action::kNone) {
        return false;
    } else {
        ExecuteAction(action, parent, *this);

        // if (key == gui::Key::kA && modifiers == gui::kPrimaryModifier) {
        //     parent.createNWindows(25);
        // }
        // if (key == gui::Key::kB && modifiers == gui::kPrimaryModifier) {
        //     parent.destroyAllWindows();
        // }

        // if (key == gui::Key::kC && modifiers == gui::kPrimaryModifier) {
        //     close();
        // }

        return true;
    }
}

void EditorWindow::onInsertText(std::string_view text) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    tab->buffer.insert(tab->end_caret.line, tab->end_caret.column, text);
    redraw();
}

void EditorWindow::onAction(gui::Action action) {
    std::unique_ptr<EditorTab>& tab = tabs.at(tab_index);
    ExecuteGuiAction(action, tab.get());
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::onDarkModeToggle() {
    reloadColorScheme();
}
