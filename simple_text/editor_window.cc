#include "editor_window.h"
#include "gui/horizontal_layout_widget.h"
#include "gui/side_bar_widget.h"
#include "gui/status_bar_widget.h"
#include "gui/tab_bar_widget.h"
#include "gui/text_view_widget.h"
#include "gui/vertical_layout_widget.h"
#include "renderer/renderer.h"
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

Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod
tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse
cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non
proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
)";
}

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent}, wid{wid}, parent{parent}, main_widget{new gui::VerticalLayoutWidget{{}}} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    using namespace gui;
    using namespace renderer;

    int tab_bar_height = 32 * 2;
    int side_bar_width = 200 * 2;
    int status_bar_height = 22 * 2;

    std::unique_ptr<ContainerWidget> horizontal_layout{new HorizontalLayoutWidget({})};
    std::unique_ptr<ContainerWidget> vertical_layout{new VerticalLayoutWidget({})};
    std::unique_ptr<Widget> side_bar{new SideBarWidget({side_bar_width, 0})};
    std::unique_ptr<Widget> tab_bar{new TabBarWidget({0, tab_bar_height})};
    std::unique_ptr<TextViewWidget> text_view{new TextViewWidget({})};
    std::unique_ptr<Widget> status_bar{new StatusBarWidget({0, status_bar_height})};

    text_view->setContents(sample_text);

    horizontal_layout->addChild(std::move(side_bar));
    vertical_layout->addChild(std::move(tab_bar));
    vertical_layout->addChild(std::move(text_view));
    horizontal_layout->addChild(std::move(vertical_layout));
    main_widget->addChild(std::move(horizontal_layout));
    main_widget->addChild(std::move(status_bar));
}

void EditorWindow::onDraw(int width, int height) {
    {
        PROFILE_BLOCK("render");
        // TODO: Move Renderer::flush() to GUI toolkit instead of calling this directly.
        main_widget->draw({width, height}, {0, 0});
        renderer::g_renderer->flush({width, height});
    }
}

void EditorWindow::onResize(int width, int height) {
    redraw();
}

void EditorWindow::onScroll(int dx, int dy) {
    main_widget->scroll({dx, dy});
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    main_widget->leftMouseDown({mouse_x, mouse_y}, {0, 0});
    redraw();
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    main_widget->leftMouseDown({mouse_x, mouse_y}, {0, 0});
    redraw();
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
