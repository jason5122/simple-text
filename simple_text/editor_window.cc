#include "editor_window.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/horizontal_layout_widget.h"
#include "gui/widget/padding_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "gui/widget/status_bar_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/text_view_widget.h"
#include "gui/widget/vertical_layout_widget.h"
#include "simple_text/editor_app.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"

using namespace gui;

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
    : Window{parent, width, height},
      wid{wid},
      parent{parent},
      main_widget{new VerticalLayoutWidget{}} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);

    int tab_bar_height = 29 * 2;
    int side_bar_width = 250 * 2;
    int status_bar_height = 22 * 2;

    // Main widgets.
    std::unique_ptr<ContainerWidget> horizontal_layout{new HorizontalLayoutWidget{}};
    std::unique_ptr<ContainerWidget> vertical_layout{new VerticalLayoutWidget{}};
    std::unique_ptr<TextViewWidget> text_view{new TextViewWidget{}};

    // These don't have default constructors since they are not intended to be main widgets.
    std::unique_ptr<Widget> side_bar{new SideBarWidget({side_bar_width, height})};
    std::unique_ptr<Widget> tab_bar{new TabBarWidget({width, tab_bar_height})};
    std::unique_ptr<Widget> status_bar{new StatusBarWidget({width, status_bar_height})};

    // Leave padding between window title bar and tab.
    constexpr Rgba tab_bar_color{190, 190, 190, 255};
    std::unique_ptr<Widget> padding{new PaddingWidget({0, 3 * 2}, tab_bar_color)};

    text_view->setContents(sample_text);

    // TODO: Temporary hack. Consider implementing this fully.
    text_view_widget = text_view.get();

    horizontal_layout->addChildStart(std::move(side_bar));
    vertical_layout->addChildStart(std::move(padding));
    vertical_layout->addChildStart(std::move(tab_bar));
    vertical_layout->setMainWidget(std::move(text_view));
    horizontal_layout->setMainWidget(std::move(vertical_layout));
    main_widget->setMainWidget(std::move(horizontal_layout));
    main_widget->addChildEnd(std::move(status_bar));
}

void EditorWindow::onDraw(int width, int height) {
    {
        // PROFILE_BLOCK("Total render time");
        // TODO: Move Renderer::flush() to GUI toolkit instead of calling this directly.
        main_widget->draw();
        Renderer::instance().flush({width, height});
    }
}

void EditorWindow::onResize(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);
    main_widget->layout();
    redraw();
}

void EditorWindow::onScroll(int mouse_x, int mouse_y, int dx, int dy) {
    main_widget->scroll({mouse_x, mouse_y}, {dx, dy});
    main_widget->layout();
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    // drag_start_widget = main_widget->getWidgetAtPosition({mouse_x, mouse_y});

    // if (drag_start_widget) {
    //     drag_start_widget->leftMouseDown({mouse_x, mouse_y});
    //     redraw();
    // }

    if (text_view_widget->hitTest({mouse_x, mouse_y})) {
        drag_start_widget = text_view_widget;

        if (text_view_widget) {
            text_view_widget->leftMouseDown({mouse_x, mouse_y});
            redraw();
        }
    }
}

void EditorWindow::onLeftMouseUp() {
    drag_start_widget = nullptr;
}

void EditorWindow::onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {
    if (drag_start_widget) {
        drag_start_widget->leftMouseDrag({mouse_x, mouse_y});
        redraw();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}
