#include "editor_window.h"

#include "app/menu.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/container/horizontal_layout_widget.h"
#include "gui/widget/container/vertical_layout_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "gui/widget/status_bar_widget.h"
#include "simple_text/editor_app.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include "util/std_print.h"

using namespace gui;

namespace {

constexpr auto operator*(const std::string_view& sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) {
        result += sv;
    }
    return result;
}

const std::string kSampleText =
    R"(Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut
labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris
nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit
esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt
in culpa qui officia deserunt mollit anim id est laborum.

)";

const std::string kLongLine =
    "This is a long long long long long long long long long long long long long long long long "
    "long long long long long long long long long long long long long long long long long long "
    "long long long long long long long long long long long long long long long long long long "
    "long long line.";

const std::string kUnicode =
    R"(˚¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£
¬ß∆ƒ¬∆ß∂ƒÒÔÏÍÎ˜´Ò‰„´‰€‹‹··ºœ™£™º¡£

🥲🥲🥲🥲🥲🥲)";

}  // namespace

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent, width, height},
      wid{wid},
      parent{parent},
      main_widget{new VerticalLayoutWidget{}} {}

void EditorWindow::onOpenGLActivate(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);

    editor_widget = std::make_shared<EditorWidget>();
    // editor_widget->addTab("hello.txt", "Hello world!\nhi there");
    // editor_widget->addTab("unicode.txt", kUnicode);
    // editor_widget->addTab("long_line.txt", kLongLine * 50 + kSampleText);
    // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);

    auto* text_view = editor_widget->currentWidget();
    text_view->insertText("⌚..⌛⏩..⏬☂️..☃️");

    // Main widgets.
    std::shared_ptr<LayoutWidget> horizontal_layout{new HorizontalLayoutWidget{}};
    std::shared_ptr<LayoutWidget> vertical_layout{new VerticalLayoutWidget{}};

    // These don't have default constructors since they are not intended to be main widgets.
    std::shared_ptr<Widget> side_bar{new SideBarWidget({kSideBarWidth, height})};
    std::shared_ptr<Widget> status_bar{new StatusBarWidget({width, kStatusBarHeight})};

    horizontal_layout->addChildStart(side_bar);
    vertical_layout->setMainWidget(editor_widget);
    horizontal_layout->setMainWidget(vertical_layout);
    main_widget->setMainWidget(horizontal_layout);
    main_widget->addChildEnd(status_bar);
}

void EditorWindow::onDraw(int width, int height) {
    PROFILE_BLOCK("Total render time");

    updateCursorStyle();

    auto mouse_pos = mousePosition();
    std::optional<Point> point{};
    if (mouse_pos) {
        auto [mouse_x, mouse_y] = mouse_pos.value();
        point = Point{mouse_x, mouse_y};
    }

    main_widget->layout();
    main_widget->mousePositionChanged(point);
    main_widget->draw(point);

    Renderer::instance().flush({width, height});
}

void EditorWindow::onResize(int width, int height) {
    main_widget->setWidth(width);
    main_widget->setHeight(height);
    redraw();
}

void EditorWindow::onScroll(int mouse_x, int mouse_y, int dx, int dy) {
    main_widget->scroll({mouse_x, mouse_y}, {dx, dy});
    redraw();
}

void EditorWindow::onLeftMouseDown(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    // createMenuDebug();

    drag_start_widget = main_widget->getWidgetAtPosition({mouse_x, mouse_y});
    if (drag_start_widget) {
        drag_start_widget->leftMouseDown({mouse_x, mouse_y}, modifiers, click_type);
        redraw();
    }
}

void EditorWindow::onLeftMouseUp() {
    drag_start_widget = nullptr;
}

void EditorWindow::onLeftMouseDrag(int mouse_x,
                                   int mouse_y,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    if (drag_start_widget) {
        drag_start_widget->leftMouseDrag({mouse_x, mouse_y}, modifiers, click_type);
        redraw();
    }
}

void EditorWindow::onRightMouseDown(int mouse_x,
                                    int mouse_y,
                                    app::ModifierKey modifiers,
                                    app::ClickType click_type) {
    auto mouse_pos = mousePositionRaw();
    if (mouse_pos) {
        parent.setCursorStyle(app::App::CursorStyle::kArrow);

        app::Menu menu;
        std::string temp = "TODO: Change this";
        menu.addItem(temp);
        auto selected_index = menu.show(mouse_pos.value());
        if (selected_index) {
            std::println("Selected menu index = {}", selected_index.value());
        } else {
            std::println("Menu was closed without a selection.");
        }
    }
}

void EditorWindow::onMouseMove() {
    updateCursorStyle();

    bool should_redraw = false;
    auto mouse_pos = mousePosition();
    if (mouse_pos) {
        auto [mouse_x, mouse_y] = mouse_pos.value();
        should_redraw = main_widget->mousePositionChanged(Point{mouse_x, mouse_y});
    } else {
        should_redraw = main_widget->mousePositionChanged(std::nullopt);
    }

    if (should_redraw) {
        redraw();
    }
}

void EditorWindow::onMouseExit() {
    onMouseMove();
}

bool EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    // std::println("key = {}, modifiers = {}", key, modifiers);

    bool handled = false;
    if (key == app::Key::kJ && modifiers == app::kPrimaryModifier) {
        editor_widget->prevIndex();
        handled = true;
    } else if (key == app::Key::kK && modifiers == app::kPrimaryModifier) {
        editor_widget->nextIndex();
        handled = true;
    } else if (key == app::Key::k1 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(0);
        handled = true;
    } else if (key == app::Key::k2 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(1);
        handled = true;
    } else if (key == app::Key::k3 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(2);
        handled = true;
    } else if (key == app::Key::k4 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(3);
        handled = true;
    } else if (key == app::Key::k5 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(4);
        handled = true;
    } else if (key == app::Key::k6 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(5);
        handled = true;
    } else if (key == app::Key::k7 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(6);
        handled = true;
    } else if (key == app::Key::k8 && modifiers == app::kPrimaryModifier) {
        editor_widget->setIndex(7);
        handled = true;
    } else if (key == app::Key::k9 && modifiers == app::kPrimaryModifier) {
        editor_widget->lastIndex();
        handled = true;
    } else if (key == app::Key::kA && modifiers == app::kPrimaryModifier) {
        editor_widget->currentWidget()->selectAll();
        handled = true;
    } else if (key == app::Key::kN && modifiers == app::kPrimaryModifier) {
        editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        handled = true;
    } else if (key == app::Key::kN &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createWindow();
        handled = true;
    } else if (key == app::Key::kW && modifiers == app::kPrimaryModifier) {
        // If we are dragging on the current TextViewWidget, invalidate the pointer to prevent a
        // use-after-free crash.
        if (drag_start_widget == editor_widget->currentWidget()) {
            drag_start_widget = nullptr;
        }

        editor_widget->removeTab(editor_widget->getCurrentIndex());
        handled = true;
    } else if (key == app::Key::kW &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        // Immediately exit this function and don't let redraw() get called! This window's smart
        // pointer will be deallocated after `close()`.
        close();
        return true;
    } else if (key == app::Key::kC && modifiers == app::kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        parent.setClipboardString(text_view->getSelectionText());
        handled = true;
    } else if (key == app::Key::kV && modifiers == app::kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        text_view->insertText(parent.getClipboardString());
        handled = true;
    } else if (key == app::Key::kX && modifiers == app::kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        parent.setClipboardString(text_view->getSelectionText());
        text_view->leftDelete();
        handled = true;
    } else if (key == app::Key::kO && modifiers == app::kPrimaryModifier) {
        auto path = openFilePicker();
        if (path) {
            editor_widget->openFile(*path);
            editor_widget->lastIndex();
            handled = true;
        }
    } else if (key == app::Key::kZ && modifiers == app::kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        text_view->undo();
        handled = true;
    } else if (key == app::Key::kZ &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        auto* text_view = editor_widget->currentWidget();
        text_view->redo();
        handled = true;
    } else if (key == app::Key::kBackspace) {
        auto* text_view = editor_widget->currentWidget();
        text_view->leftDelete();
        handled = true;
    } else if (key == app::Key::kEnter) {
        auto* text_view = editor_widget->currentWidget();
        text_view->insertText("\n");
        handled = true;
    } else if (key == app::Key::kTab) {
        auto* text_view = editor_widget->currentWidget();
        text_view->insertText("    ");
        handled = true;
    }

    if (handled) {
        redraw();
    }
    return handled;
}

void EditorWindow::onInsertText(std::string_view text) {
    {
        PROFILE_BLOCK("EditorWindow::onInsertText()");
        TextViewWidget* widget = editor_widget->currentWidget();
        if (widget) widget->insertText(text);
    }
    redraw();
}

void EditorWindow::onAction(app::Action action, bool extend) {
    bool handled = false;
    {
        PROFILE_BLOCK("EditorWindow::onAction()");
        auto* text_view = editor_widget->currentWidget();
        if (action == app::Action::kMoveForwardByCharacters) {
            text_view->move(gui::MoveBy::kCharacters, true, extend);
            handled = true;
        }
        if (action == app::Action::kMoveBackwardByCharacters) {
            text_view->move(gui::MoveBy::kCharacters, false, extend);
            handled = true;
        }
        if (action == app::Action::kMoveForwardByLines) {
            text_view->move(gui::MoveBy::kLines, true, extend);
            handled = true;
        }
        if (action == app::Action::kMoveBackwardByLines) {
            text_view->move(gui::MoveBy::kLines, false, extend);
            handled = true;
        }
        if (action == app::Action::kMoveForwardByWords) {
            text_view->move(gui::MoveBy::kWords, true, extend);
            handled = true;
        }
        if (action == app::Action::kMoveBackwardByWords) {
            text_view->move(gui::MoveBy::kWords, false, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToBOL) {
            text_view->moveTo(gui::MoveTo::kBOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToEOL) {
            text_view->moveTo(gui::MoveTo::kEOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToHardBOL) {
            text_view->moveTo(gui::MoveTo::kHardBOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToHardEOL) {
            text_view->moveTo(gui::MoveTo::kHardEOL, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToBOF) {
            text_view->moveTo(gui::MoveTo::kBOF, extend);
            handled = true;
        }
        if (action == app::Action::kMoveToEOF) {
            text_view->moveTo(gui::MoveTo::kEOF, extend);
            handled = true;
        }
        if (action == app::Action::kInsertNewline) {
            text_view->insertText("\n");
            handled = true;
        }
        if (action == app::Action::kInsertTab) {
            text_view->insertText("    ");
            handled = true;
        }
        if (action == app::Action::kLeftDelete) {
            text_view->leftDelete();
            handled = true;
        }
        if (action == app::Action::kRightDelete) {
            text_view->rightDelete();
            handled = true;
        }
        if (action == app::Action::kDeleteWordForward) {
            text_view->deleteWord(true);
            handled = true;
        }
        if (action == app::Action::kDeleteWordBackward) {
            text_view->deleteWord(false);
            handled = true;
        }
    }

    if (handled) {
        redraw();
    }
}

void EditorWindow::onAppAction(app::AppAction action) {
    if (action == app::AppAction::kNewFile) {
        editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        redraw();
    }
    if (action == app::AppAction::kNewWindow) {
        parent.createWindow();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::updateCursorStyle() {
    using CursorStyle = app::App::CursorStyle;

    auto mouse_pos = mousePosition();

    // Update cursor style.
    // Case 1: Dragging operation in progress.
    if (drag_start_widget) {
        if (drag_start_widget->getCursorStyle() == gui::CursorStyle::kArrow) {
            parent.setCursorStyle(CursorStyle::kArrow);
        } else {
            parent.setCursorStyle(CursorStyle::kIBeam);
        }
    }
    // Case 2: Mouse position is within window.
    else if (mouse_pos) {
        auto [mouse_x, mouse_y] = mouse_pos.value();
        Widget* hovered_widget = main_widget->getWidgetAtPosition(Point{mouse_x, mouse_y});
        if (hovered_widget) {
            // std::println("{}", *hovered_widget);
            if (hovered_widget->getCursorStyle() == gui::CursorStyle::kArrow) {
                parent.setCursorStyle(CursorStyle::kArrow);
            } else {
                parent.setCursorStyle(CursorStyle::kIBeam);
            }
        }
    }
    // Case 3: Mouse position is outside of window.
    else {
        parent.setCursorStyle(CursorStyle::kArrow);
    }
}
