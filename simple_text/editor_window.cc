#include "editor_window.h"
#include "gui/side_bar_widget.h"
#include "gui/tab_bar_widget.h"
#include "gui/text_view_widget.h"
#include "gui/vertical_layout_widget.h"
#include "simple_text/editor_app.h"

// TODO: Debug; remove this.
#include "util/profile_util.h"

namespace {
const std::string sample_text =
    R"(Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
)";
}

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent}, wid{wid}, parent{parent}, main_widget{{}} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    using namespace gui;
    using namespace renderer;

    int tab_bar_height = 30 * 2;
    int side_bar_width = 200 * 2;

    std::unique_ptr<VerticalLayoutWidget> vertical_layout{new VerticalLayoutWidget({})};
    std::unique_ptr<Widget> side_bar{new SideBarWidget(parent.renderer, {side_bar_width, 0})};
    std::unique_ptr<Widget> tab_bar{new TabBarWidget(parent.renderer, {0, tab_bar_height})};
    std::unique_ptr<TextViewWidget> text_view{new TextViewWidget(parent.renderer, {})};

    text_view->setContents(sample_text);

    main_widget.addChild(std::move(side_bar));
    vertical_layout->addChild(std::move(tab_bar));
    vertical_layout->addChild(std::move(text_view));
    main_widget.addChild(std::move(vertical_layout));
}

void EditorWindow::onDraw(int width, int height) {
    {
        PROFILE_BLOCK("render");
        // TODO: Move Renderer::flush() to GUI toolkit instead of calling this directly.
        main_widget.draw({width, height}, {0, 0});
        parent.renderer->flush({width, height});
    }
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    main_widget.scroll({dx, dy});
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    // int line_number_offset = 100;
    // renderer::Point mouse{
    //     .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
    //     .y = mouse_y + scroll_offset.y - 30 * 2,
    // };

    // parent.renderer->getMovement().setCaretInfo(buffer, mouse, end_caret);

    // redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    // int line_number_offset = 100;
    // renderer::Point mouse{
    //     .x = mouse_x + scroll_offset.x - 200 * 2 - line_number_offset,
    //     .y = mouse_y + scroll_offset.y - 30 * 2,
    // };

    // parent.renderer->getMovement().setCaretInfo(buffer, mouse, end_caret);

    // redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
