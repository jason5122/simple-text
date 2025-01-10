#include "editor_window.h"

#include "gui/renderer/renderer.h"
#include "gui/widget/container/horizontal_layout_widget.h"
#include "gui/widget/container/horizontal_resizing_widget.h"
#include "gui/widget/container/vertical_resizing_widget.h"
#include "gui/widget/debug/atlas_widget.h"
#include "gui/widget/find_panel_widget.h"
#include "simple_text/editor_app.h"

#include <string>

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <fmt/base.h>
#include <fmt/format.h>

namespace gui {

namespace {

const std::string kCppExample =
#include "sample_code.txt"
    ;

}  // namespace

EditorWindow::EditorWindow(EditorApp& parent, int width, int height, int wid)
    : WindowWidget(parent, width, height),
      wid(wid),
      parent(parent),
      main_widget(std::make_unique<VerticalResizingWidget>()),
      editor_widget(new EditorWidget(
          parent.main_font_id, parent.ui_font_small_id, parent.panel_close_image_id)),
      status_bar(new StatusBarWidget(kMinStatusBarHeight, parent.ui_font_small_id)),
      side_bar(new SideBarWidget(kSideBarWidth)) {

    // Set initial focused widget to EditorWidget.
    focused_widget = editor_widget;
}

// On GTK, size isn't available upon realization. This is fine, just don't rely on `size` here!
void EditorWindow::onOpenGLActivate() {
    using namespace std::literals;
    auto* text_view = editor_widget->currentWidget();
    // text_view->insertText("⌚..⌛⏩..⏬☂️..☃️");
    text_view->insertText(kCppExample);
    // TODO: Fix these cases on Pango. Core Text has been fixed.
    // text_view->insertText("\n꣰");
    // text_view->insertText("ᩣᩤᩥᩦᩧᩨᩩᩪᩫᩬᩭ");
    // TODO: Fix these cases with all font rasterizers.
    // text_view->insertText("⃒⃓⃘⃙⃚⃑⃔⃕⃖⃗⃛⃜⃝⃞⃟⃠⃡⃢⃣⃤⃥");
    // text_view->insertText("̴̵̶̷̸̡̢̧̨̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̽̾̿̀́͂̓̈́͆͊͋͌ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡Ͱ");

    // TODO: Maybe use this case to optimize stuff. This could detect if we're doing line/col ->
    // offset conversions too much, for example.
    // text_view->insertText(kLongLine * 10000);

    // Main widgets.
    auto horizontal_layout = std::make_unique<HorizontalResizingWidget>();
    horizontal_layout->addChildStart(std::unique_ptr<SideBarWidget>(side_bar));
    horizontal_layout->setMainWidget(std::unique_ptr<EditorWidget>(editor_widget));
    constexpr bool kShowAtlas = false;
    if constexpr (kShowAtlas) {
        auto atlas_widget = std::make_unique<AtlasWidget>();
        horizontal_layout->addChildEnd(std::move(atlas_widget));
    }

    main_widget->setMainWidget(std::move(horizontal_layout));
    status_bar->setResizable(false);
    main_widget->addChildEnd(std::unique_ptr<StatusBarWidget>(status_bar));

    auto find_panel_widget = std::make_unique<FindPanelWidget>(
        parent.main_font_id, parent.ui_font_regular_id, parent.icon_regex_image_id,
        parent.icon_case_sensitive_image_id, parent.icon_whole_word_image_id,
        parent.icon_wrap_image_id, parent.icon_in_selection_id, parent.icon_highlight_matches_id,
        parent.panel_close_image_id);
    main_widget->addChildEnd(std::move(find_panel_widget));
}

void EditorWindow::draw() {
    // PROFILE_BLOCK("Total render time");

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

    // TODO: Refactor this.
    // if (requested_frames > 0) {
    //     setAutoRedraw(true);
    // } else {
    //     setAutoRedraw(false);
    // }
}

void EditorWindow::onFrame(int64_t ms) {
    // if (is_side_bar_animating) {
    //     int64_t d = ms - last_ms;
    //     int64_t numerator = d * kRatePerSec + ms_err;
    //     ms_err += numerator % 1000;
    //     int64_t num_updates = numerator / 1000;

    //     int width = side_bar->getWidth();
    //     if (is_side_bar_open && width > target_width) {
    //         int new_width = width - kRatePerSec * num_updates;
    //         side_bar->setWidth(std::max(new_width, target_width));

    //         if (side_bar->getWidth() == target_width) {
    //             is_side_bar_open = false;
    //             is_side_bar_animating = false;
    //             ms_err = 0;
    //             setAutoRedraw(false);
    //         }
    //     } else if (!is_side_bar_open && width < target_width) {
    //         int new_width = width + kRatePerSec * num_updates;
    //         side_bar->setWidth(std::min(new_width, target_width));

    //         if (side_bar->getWidth() == target_width) {
    //             is_side_bar_open = true;
    //             is_side_bar_animating = false;
    //             ms_err = 0;
    //             setAutoRedraw(false);
    //         }
    //     }
    // }

    // fmt::println("{} {}", vel_x, vel_y);
    // if (vel_x > 0 || vel_y > 0) {
    //     main_widget->scroll(last_mouse_pos, {vel_x, vel_y});
    //     vel_x = std::max(vel_x - kDecelFriction, 0);
    //     vel_y = std::max(vel_y - kDecelFriction, 0);
    // } else if (vel_x < 0 || vel_y < 0) {
    //     main_widget->scroll(last_mouse_pos, {vel_x, vel_y});
    //     vel_x = std::min(vel_x + kDecelFriction, 0);
    //     vel_y = std::min(vel_y + kDecelFriction, 0);
    // } else {
    //     setAutoRedraw(false);
    // }

    redraw();

    --requested_frames;
    if (requested_frames == 0) {
        setAutoRedraw(false);
    }

    // last_ms = ms;
}

// TODO: Verify that resize is always called on all platforms when the window is created.
// TODO: Verify that resizes are followed by redraw calls in the GUI framework.
void EditorWindow::layout() {
    main_widget->setSize(size);
    // side_bar->setMinimumWidth(100);
    // side_bar->setMaximumWidth(size.width - 100);
}

void EditorWindow::performScroll(const Point& mouse_pos, const Delta& delta) {
    main_widget->mousePositionChanged(mouse_pos);
    main_widget->performScroll(mouse_pos, delta);

    // https://zed.dev/blog/120fps
    requested_frames = framesPerSecond();
    setAutoRedraw(true);
    redraw();
}

void EditorWindow::leftMouseDown(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {
    dragged_widget = main_widget->widgetAt(mouse_pos);
    if (dragged_widget) {
        if (dragged_widget->canBeFocused()) {
            focused_widget = dragged_widget;
        }

        dragged_widget->leftMouseDown(mouse_pos, modifiers, click_type);
        // TODO: See if we should call `updateCursorStyle()` here.
        // TODO: Not all widgets should initiate a drag.
        redraw();
    }

    if (dragged_widget) {
        fmt::println("dragged widget: {}", dragged_widget->className());
    } else {
        fmt::println("no dragged widget");
    }
    if (focused_widget) {
        fmt::println("focused widget: {}", focused_widget->className());
    } else {
        fmt::println("no focused widget");
    }
}

void EditorWindow::leftMouseDrag(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {
    if (dragged_widget) {
        dragged_widget->leftMouseDrag(mouse_pos, modifiers, click_type);
        // TODO: See if we should call `updateCursorStyle()` here.
        // TODO: Not all widgets should initiate a drag.
        redraw();
    }
}

void EditorWindow::leftMouseUp(const Point& mouse_pos) {
    if (dragged_widget) {
        dragged_widget->leftMouseUp(mouse_pos);
        redraw();
    }
    dragged_widget = nullptr;
    updateCursorStyle(mouse_pos);
}

void EditorWindow::rightMouseDown(const Point& mouse_pos,
                                  ModifierKey modifiers,
                                  ClickType click_type) {
    // auto mouse_pos = mousePositionRaw();
    // if (mouse_pos) {
    //     setCursorStyle(CursorStyle::kArrow);

    //     Menu menu;
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
bool EditorWindow::mousePositionChanged(const std::optional<Point>& mouse_pos) {
    updateCursorStyle(mouse_pos);

    if (main_widget->mousePositionChanged(mouse_pos)) {
        redraw();
        return true;
    } else {
        return false;
    }
}

// TODO: At this point, this is GTK-specific. Make the deceleration feel more natural.
void EditorWindow::onScrollDecelerate(const Point& mouse_pos, const Delta& delta) {
    vel_x = delta.dx;
    vel_y = delta.dy;
    last_mouse_pos = mouse_pos;

    vel_x /= 16;
    vel_y /= 16;

    setAutoRedraw(true);
    redraw();
}

bool EditorWindow::onKeyDown(Key key, ModifierKey modifiers) {
    // fmt::println("key = {}, modifiers = {}", key, modifiers);

    bool handled = false;
    if (key == Key::kJ && modifiers == kPrimaryModifier) {
        editor_widget->prevIndex();
        handled = true;
    } else if (key == Key::kK && modifiers == kPrimaryModifier) {
        editor_widget->nextIndex();
        handled = true;
    } else if (key == Key::k1 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(0);
        handled = true;
    } else if (key == Key::k2 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(1);
        handled = true;
    } else if (key == Key::k3 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(2);
        handled = true;
    } else if (key == Key::k4 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(3);
        handled = true;
    } else if (key == Key::k5 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(4);
        handled = true;
    } else if (key == Key::k6 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(5);
        handled = true;
    } else if (key == Key::k7 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(6);
        handled = true;
    } else if (key == Key::k8 && modifiers == kPrimaryModifier) {
        editor_widget->setIndex(7);
        handled = true;
    } else if (key == Key::k9 && modifiers == kPrimaryModifier) {
        editor_widget->lastIndex();
        handled = true;
    } else if (key == Key::kA && modifiers == kPrimaryModifier) {
        editor_widget->currentWidget()->selectAll();
        handled = true;
    } else if (key == Key::kN && modifiers == kPrimaryModifier) {
        PROFILE_BLOCK("Add new tab (modifier key)");
        // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        editor_widget->addTab("untitled", "");
        handled = true;
    } else if (key == Key::kN && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        parent.createWindow();
        handled = true;
    } else if (key == Key::kW && modifiers == kPrimaryModifier) {
        // If we are dragging on the current TextViewWidget, invalidate the pointer to prevent a
        // use-after-free crash.
        if (dragged_widget == editor_widget->currentWidget()) {
            dragged_widget = nullptr;
        }

        editor_widget->removeTab(editor_widget->getCurrentIndex());
        handled = true;
    } else if (key == Key::kW && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        // Immediately exit this function and don't let any other methods get called! This window's
        // smart pointer will be deallocated after `close()`.
        close();
        return true;
    } else if (key == Key::kC && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        parent.setClipboardString(text_view->getSelectionText());
        handled = true;
    } else if (key == Key::kV && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        text_view->insertText(parent.getClipboardString());
        handled = true;
    } else if (key == Key::kX && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        parent.setClipboardString(text_view->getSelectionText());
        text_view->leftDelete();
        handled = true;
    } else if (key == Key::kO && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        auto path = openFilePicker();
        if (path) {
            editor_widget->openFile(*path);
            editor_widget->lastIndex();
            handled = true;
        }
    } else if (key == Key::kZ && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        text_view->undo();
        handled = true;
    } else if (key == Key::kZ && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        auto* text_view = editor_widget->currentWidget();
        text_view->redo();
        handled = true;
    } else if (key == Key::kBackspace && modifiers == ModifierKey::kNone) {
        auto* text_view = editor_widget->currentWidget();
        text_view->leftDelete();
        handled = true;
    } else if (key == Key::kEnter && modifiers == ModifierKey::kNone) {
        // auto* text_view = editor_widget->currentWidget();
        // text_view->insertText("\n");
        focused_widget->insertText("\n");
        handled = true;
    } else if (key == Key::kTab && modifiers == ModifierKey::kNone) {
        auto* text_view = editor_widget->currentWidget();
        // TODO: Don't hard code this.
        text_view->insertText("    ");
        handled = true;
    } else if (key == Key::kQ && modifiers == kPrimaryModifier) {
        parent.quit();
    } else if (key == Key::kF && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->currentWidget();
        // TODO: Don't hard code this.
        text_view->find("needle");
        handled = true;
    }

    // TODO: Remove this.
    // if (key == Key::kU && modifiers == kPrimaryModifier) {
    //     performScroll({500, 500}, {0, -1});
    //     handled = true;
    // }
    // if (key == Key::kI && modifiers == kPrimaryModifier) {
    //     performScroll({500, 500}, {0, 1});
    //     handled = true;
    // }

    // TODO: Refactor this.
    // if (key == Key::kI && modifiers == kPrimaryModifier) {

    //     int side_bar_width = side_bar->getWidth();
    //     if (is_side_bar_animating) {
    //         is_side_bar_open = !is_side_bar_open;
    //         ms_err = 0;
    //     }
    //     if (is_side_bar_open) {
    //         target_width = 0;
    //     } else {
    //         target_width = 500;
    //     }
    //     is_side_bar_animating = true;
    //     setAutoRedraw(true);

    //     handled = true;
    // }

    // TODO: Clean this up.
    if (key == Key::kMinus && modifiers == kPrimaryModifier) {
        auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.metrics(parent.main_font_id);

        int new_font_size = std::max(metrics.font_size - 2, 8 * 2);
        fmt::println("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resizeFont(parent.main_font_id, new_font_size);

        editor_widget->updateFontId(parent.main_font_id);

        handled = true;
    } else if (key == Key::kEqual && modifiers == kPrimaryModifier) {
        auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.metrics(parent.main_font_id);

        int new_font_size = std::min(metrics.font_size + 2, 128 * 2);
        fmt::println("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resizeFont(parent.main_font_id, new_font_size);

        editor_widget->updateFontId(parent.main_font_id);

        handled = true;
    } else if (key == Key::k0 && modifiers == kPrimaryModifier) {
        auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.metrics(parent.main_font_id);

        int new_font_size = parent.kMainFontSize;
        fmt::println("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resizeFont(parent.main_font_id, new_font_size);

        editor_widget->updateFontId(parent.main_font_id);

        handled = true;
    }

    if (handled) {
        redraw();
    }
    return handled;
}

void EditorWindow::onInsertText(std::string_view text) {
    // if (auto widget = editor_widget->currentWidget()) {

    if (focused_widget) {
        focused_widget->insertText(text);
        redraw();
        // auto editor_widget = static_cast<EditorWidget*>(focused_widget);
        // if (auto widget = editor_widget->currentWidget()) {
        //     widget->insertText(text);
        //     redraw();
        // }
    }
}

void EditorWindow::onAction(Action action, bool extend) {
    PROFILE_BLOCK("EditorWindow::onAction()");

    bool handled = false;
    auto* text_view = editor_widget->currentWidget();
    if (action == Action::kMoveForwardByCharacters) {
        text_view->move(gui::MoveBy::kCharacters, true, extend);
        handled = true;
    }
    if (action == Action::kMoveBackwardByCharacters) {
        text_view->move(gui::MoveBy::kCharacters, false, extend);
        handled = true;
    }
    if (action == Action::kMoveForwardByLines) {
        text_view->move(gui::MoveBy::kLines, true, extend);
        handled = true;
    }
    if (action == Action::kMoveBackwardByLines) {
        text_view->move(gui::MoveBy::kLines, false, extend);
        handled = true;
    }
    if (action == Action::kMoveForwardByWords) {
        text_view->move(gui::MoveBy::kWords, true, extend);
        handled = true;
    }
    if (action == Action::kMoveBackwardByWords) {
        text_view->move(gui::MoveBy::kWords, false, extend);
        handled = true;
    }
    if (action == Action::kMoveToBOL) {
        text_view->moveTo(gui::MoveTo::kBOL, extend);
        handled = true;
    }
    if (action == Action::kMoveToEOL) {
        text_view->moveTo(gui::MoveTo::kEOL, extend);
        handled = true;
    }
    if (action == Action::kMoveToHardBOL) {
        text_view->moveTo(gui::MoveTo::kHardBOL, extend);
        handled = true;
    }
    if (action == Action::kMoveToHardEOL) {
        text_view->moveTo(gui::MoveTo::kHardEOL, extend);
        handled = true;
    }
    if (action == Action::kMoveToBOF) {
        text_view->moveTo(gui::MoveTo::kBOF, extend);
        handled = true;
    }
    if (action == Action::kMoveToEOF) {
        text_view->moveTo(gui::MoveTo::kEOF, extend);
        handled = true;
    }
    if (action == Action::kInsertNewline) {
        // text_view->insertText("\n");
        focused_widget->insertText("\n");
        handled = true;
    }
    if (action == Action::kInsertNewlineIgnoringFieldEditor) {
        // text_view->insertText("\n");
        focused_widget->insertText("\n");
        // This command is sent as the first part of the `ctrl+o` keybind. We shouldn't redraw.
        return;
    }
    if (action == Action::kInsertTab) {
        text_view->insertText("    ");
        handled = true;
    }
    if (action == Action::kLeftDelete) {
        text_view->leftDelete();
        handled = true;
    }
    if (action == Action::kRightDelete) {
        text_view->rightDelete();
        handled = true;
    }
    if (action == Action::kDeleteWordForward) {
        text_view->deleteWord(true);
        handled = true;
    }
    if (action == Action::kDeleteWordBackward) {
        text_view->deleteWord(false);
        handled = true;
    }

    if (handled) {
        redraw();
    }
}

void EditorWindow::onAppAction(AppAction action) {
    if (action == AppAction::kNewFile) {
        PROFILE_BLOCK("Add new tab (app action)");
        // editor_widget->addTab("sample_text.txt", kSampleText * 50 + kLongLine);
        // editor_widget->addTab("sample.cc", kCppExample);
        editor_widget->addTab("untitled", "");
        redraw();
    }
    if (action == AppAction::kNewWindow) {
        parent.createWindow();
    }
}

void EditorWindow::onClose() {
    parent.destroyWindow(wid);
}

void EditorWindow::updateCursorStyle(const std::optional<Point>& mouse_pos) {
    // Case 1: Dragging operation in progress.
    if (dragged_widget) {
        setCursorStyle(dragged_widget->cursorStyle());
    }
    // Case 2: Mouse position is within window.
    else if (mouse_pos) {
        if (auto hovered_widget = main_widget->widgetAt(mouse_pos.value())) {
            // fmt::println("{}", hovered_widget->className());
            setCursorStyle(hovered_widget->cursorStyle());
        } else {
            // fmt::println("No widget hovered");
            setCursorStyle(CursorStyle::kArrow);
        }
    }
    // Case 3: Mouse position is outside of window.
    else {
        setCursorStyle(CursorStyle::kArrow);
    }
}

}  // namespace gui
