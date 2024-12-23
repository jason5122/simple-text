#include "editor_window.h"

#include "app/menu.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/container/horizontal_layout_widget.h"
#include "gui/widget/container/vertical_layout_widget.h"
#include "gui/widget/debug/horizontal_resizing_widget.h"
#include "gui/widget/find_panel_widget.h"
#include "gui/widget/side_bar_widget.h"
#include "simple_text/editor_app.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <fmt/base.h>
#include <fmt/format.h>

using namespace gui;

namespace {

constexpr auto operator* [[maybe_unused]] (const std::string_view& sv, size_t times) {
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

const std::string kCppExample = R"(🥲🥲🥲🥲🥲🥲#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tree_sitter/api.h>

// Declare the `tree_sitter_json` function, which is
// implemented by the `tree-sitter-json` library.
const TSLanguage *tree_sitter_json(void);

int main() {
  // Create a parser.
  TSParser *parser = ts_parser_new();

  // Set the parser's language (JSON in this case).
  ts_parser_set_language(parser, tree_sitter_json());

  // Build a syntax tree based on source code stored in a string.
  const char *source_code = "[1, null]";
  TSTree *tree = ts_parser_parse_string(
    parser,
    NULL,
    source_code,
    strlen(source_code)
  );

  // Get the root node of the syntax tree.
  TSNode root_node = ts_tree_root_node(tree);

  // Get some child nodes.
  TSNode array_node = ts_node_named_child(root_node, 0);
  TSNode number_node = ts_node_named_child(array_node, 0);

  // Check that the nodes have the expected types.
  assert(strcmp(ts_node_type(root_node), "document") == 0);
  assert(strcmp(ts_node_type(array_node), "array") == 0);
  assert(strcmp(ts_node_type(number_node), "number") == 0);

  // Check that the nodes have the expected child counts.
  assert(ts_node_child_count(root_node) == 1);
  assert(ts_node_child_count(array_node) == 5);
  assert(ts_node_named_child_count(array_node) == 2);
  assert(ts_node_child_count(number_node) == 0);

  // Print the syntax tree as an S-expression.
  char *string = ts_node_string(root_node);
  printf("Syntax tree: %s\n", string);

  // Free all of the heap-allocated memory.
  free(string);
  ts_tree_delete(tree);
  ts_parser_delete(parser);
  return 0;
}
)";

}  // namespace

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : Window{parent, width, height},
      wid{wid},
      parent{parent},
      main_widget{std::make_shared<VerticalLayoutWidget>()} {}

void EditorWindow::onOpenGLActivate(const app::Size& size) {
    main_widget->setSize(size);

    editor_widget = std::make_shared<EditorWidget>(parent.main_font_id, parent.ui_font_id,
                                                   parent.panel_close_image_id);
    // editor_widget->addTab("hello.txt", "Hello world!\nhi there");
    // editor_widget->addTab("unicode.txt", kUnicode);
    // editor_widget->addTab("long_line.txt", kLongLine * 50 + kSampleText);
    // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);

    auto* text_view = editor_widget->currentWidget();
    // text_view->insertText("⌚..⌛⏩..⏬☂️..☃️");
    text_view->insertText(kCppExample);

    // TODO: Maybe use this case to optimize stuff. This could detect if we're doing line/col ->
    // offset conversions too much, for example.
    // text_view->insertText(kLongLine * 10000);

    // Main widgets.
    auto horizontal_layout = std::make_shared<HorizontalResizingWidget>();
    auto vertical_layout = std::make_shared<VerticalLayoutWidget>();

    // These don't have default constructors since they are not intended to be main widgets.
    auto side_bar = std::make_shared<SideBarWidget>(app::Size{kSideBarWidth, size.height});
    status_bar = std::make_shared<StatusBarWidget>(app::Size{size.width, kStatusBarHeight},
                                                   parent.ui_font_id);

    horizontal_layout->addChildStart(side_bar);
    vertical_layout->setMainWidget(editor_widget);
    horizontal_layout->setMainWidget(vertical_layout);
    main_widget->setMainWidget(horizontal_layout);
    main_widget->addChildEnd(status_bar);

    // TODO: Formalize this.
    auto find_panel_widget = std::make_shared<FindPanelWidget>(
        app::Size{size.width, kFindPanelHeight}, parent.main_font_id);
    main_widget->addChildEnd(find_panel_widget);
}

void EditorWindow::onDraw(const app::Size& size) {
    PROFILE_BLOCK("Total render time");

    // TODO: Debug use; remove this.
    auto* text_view = editor_widget->currentWidget();
    if (text_view) {
        size_t length = text_view->getSelectionLength();
        if (length > 0) {
            status_bar->setText(
                fmt::format("{} character{} selected", length, length != 1 ? "s" : ""));
        } else {
            auto [line, col] = text_view->getLineColumn();
            status_bar->setText(fmt::format("Line {}, Column {}", line + 1, col + 1));
        }
    } else {
        status_bar->setText("No file open");
    }

    main_widget->layout();
    main_widget->draw();

    Renderer::instance().flush(size);
}

void EditorWindow::onResize(const app::Size& size) {
    main_widget->setSize(size);
    // Resizes are followed by redraw calls in the GUI framework. No need to call `redraw()`.
}

void EditorWindow::onScroll(const app::Point& mouse_pos, const app::Delta& delta) {
    main_widget->mousePositionChanged(mouse_pos);
    main_widget->scroll(mouse_pos, delta);
    redraw();
}

void EditorWindow::onLeftMouseDown(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    dragged_widget = main_widget->widgetAt(mouse_pos);
    if (dragged_widget) {
        dragged_widget->leftMouseDown(mouse_pos, modifiers, click_type);
        redraw();
    }
}

void EditorWindow::onLeftMouseUp() {
    dragged_widget = nullptr;
}

void EditorWindow::onLeftMouseDrag(const app::Point& mouse_pos,
                                   app::ModifierKey modifiers,
                                   app::ClickType click_type) {
    if (dragged_widget) {
        dragged_widget->leftMouseDrag(mouse_pos, modifiers, click_type);
        redraw();
    }
}

void EditorWindow::onRightMouseDown(const app::Point& mouse_pos,
                                    app::ModifierKey modifiers,
                                    app::ClickType click_type) {
    // auto mouse_pos = mousePositionRaw();
    // if (mouse_pos) {
    //     parent.setCursorStyle(app::App::CursorStyle::kArrow);

    //     app::Menu menu;
    //     std::string temp = "TODO: Change this";
    //     menu.addItem(temp);
    //     auto selected_index = menu.show(mouse_pos.value());
    //     if (selected_index) {
    //         fmt::println("Selected menu index = {}", selected_index.value());
    //     } else {
    //         fmt::println("Menu was closed without a selection.");
    //     }
    // }
}

// This represents the mouse moving *without* being a click+drag.
void EditorWindow::onMouseMove(const app::Point& mouse_pos) {
    updateCursorStyle(mouse_pos);

    if (main_widget->mousePositionChanged(mouse_pos)) {
        redraw();
    }
}

// Mouse position is guaranteed to be outside of the window here.
void EditorWindow::onMouseExit() {
    updateCursorStyle(std::nullopt);

    if (main_widget->mousePositionChanged(std::nullopt)) {
        redraw();
    }
}

bool EditorWindow::onKeyDown(app::Key key, app::ModifierKey modifiers) {
    // fmt::println("key = {}, modifiers = {}", key, modifiers);

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
        PROFILE_BLOCK("Add new tab (modifier key)");
        // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        editor_widget->addTab("untitled", "");
        handled = true;
    } else if (key == app::Key::kN &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createWindow();
        handled = true;
    } else if (key == app::Key::kW && modifiers == app::kPrimaryModifier) {
        // If we are dragging on the current TextViewWidget, invalidate the pointer to prevent a
        // use-after-free crash.
        if (dragged_widget == editor_widget->currentWidget()) {
            dragged_widget = nullptr;
        }

        editor_widget->removeTab(editor_widget->getCurrentIndex());
        handled = true;
    } else if (key == app::Key::kW &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        // Immediately exit this function and don't let any other methods get called! This window's
        // smart pointer will be deallocated after `close()`.
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
    } else if (key == app::Key::kO &&
               modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
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
    } else if (key == app::Key::kBackspace && modifiers == app::ModifierKey::kNone) {
        auto* text_view = editor_widget->currentWidget();
        text_view->leftDelete();
        handled = true;
    } else if (key == app::Key::kEnter && modifiers == app::ModifierKey::kNone) {
        auto* text_view = editor_widget->currentWidget();
        text_view->insertText("\n");
        handled = true;
    } else if (key == app::Key::kTab && modifiers == app::ModifierKey::kNone) {
        auto* text_view = editor_widget->currentWidget();
        // TODO: Don't hard code this.
        text_view->insertText("    ");
        handled = true;
    } else if (key == app::Key::kQ && modifiers == app::kPrimaryModifier) {
        parent.quit();
    } else if (key == app::Key::kF && modifiers == app::kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        // TODO: Don't hard code this.
        text_view->find("needle");
        handled = true;
    }

    if (handled) {
        redraw();
    }
    return handled;
}

void EditorWindow::onInsertText(std::string_view text) {
    if (auto widget = editor_widget->currentWidget()) {
        widget->insertText(text);
        redraw();
    }
}

void EditorWindow::onAction(app::Action action, bool extend) {
    PROFILE_BLOCK("EditorWindow::onAction()");

    bool handled = false;
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
    if (action == app::Action::kInsertNewlineIgnoringFieldEditor) {
        text_view->insertText("\n");
        // This command is sent as the first part of the `ctrl+o` keybind. We shouldn't redraw.
        return;
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

    if (handled) {
        redraw();
    }
}

void EditorWindow::onAppAction(app::AppAction action) {
    if (action == app::AppAction::kNewFile) {
        PROFILE_BLOCK("Add new tab (app action)");
        // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        // editor_widget->addTab("sample.cc", kCppExample);
        editor_widget->addTab("untitled", "");
        redraw();
    }
    if (action == app::AppAction::kNewWindow) {
        parent.createWindow();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::updateCursorStyle(const std::optional<app::Point>& mouse_pos) {
    // Case 1: Dragging operation in progress.
    if (dragged_widget) {
        parent.setCursorStyle(dragged_widget->cursorStyle());
    }
    // Case 2: Mouse position is within window.
    else if (mouse_pos) {
        if (auto hovered_widget = main_widget->widgetAt(mouse_pos.value())) {
            // fmt::println("{}", *hovered_widget);
            parent.setCursorStyle(hovered_widget->cursorStyle());
        }
    }
    // Case 3: Mouse position is outside of window.
    else {
        parent.setCursorStyle(app::CursorStyle::kArrow);
    }
}
