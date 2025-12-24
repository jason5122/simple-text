#include "base/debug/profiler.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/container/horizontal_resizing_widget.h"
#include "gui/widget/container/vertical_resizing_widget.h"
#include "gui/widget/debug/atlas_widget.h"
#include "gui/widget/find_panel_widget.h"
#include "simple_text/editor_app.h"
#include "simple_text/editor_window.h"
#include <format>
#include <spdlog/spdlog.h>
#include <string>

namespace gui {

// TODO: Debug use; remove this.
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
void EditorWindow::on_opengl_activate() {
    using namespace std::literals;
    auto* text_view = editor_widget->current_widget();
    // text_view->insertText("⌚..⌛⏩..⏬☂️..☃️");
    text_view->insert_text(kCppExample);
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
    // horizontal_layout->addChildStart(std::unique_ptr<SideBarWidget>(side_bar));
    horizontal_layout->set_main_widget(std::unique_ptr<EditorWidget>(editor_widget));
    constexpr bool kShowAtlas = false;
    if constexpr (kShowAtlas) {
        auto atlas_widget = std::make_unique<AtlasWidget>();
        horizontal_layout->add_child_end(std::move(atlas_widget));
    }

    main_widget->set_main_widget(std::move(horizontal_layout));
    status_bar->set_resizable(false);
    main_widget->add_child_end(std::unique_ptr<StatusBarWidget>(status_bar));

    auto find_panel_widget = std::make_unique<FindPanelWidget>(
        parent.main_font_id, parent.ui_font_regular_id, parent.icon_regex_image_id,
        parent.icon_case_sensitive_image_id, parent.icon_whole_word_image_id,
        parent.icon_wrap_image_id, parent.icon_in_selection_id, parent.icon_highlight_matches_id,
        parent.panel_close_image_id);
    main_widget->add_child_end(std::move(find_panel_widget));
}

void EditorWindow::draw() {
    // auto p = base::Profiler{"Total render time"};

    // TODO: Debug use; remove this.
    auto* text_view = editor_widget->current_widget();
    if (text_view) {
        size_t length = text_view->get_selection_length();
        if (length > 0) {
            status_bar->set_text(
                std::format("{} character{} selected", length, length != 1 ? "s" : ""));
        } else {
            auto [line, col] = text_view->get_line_column();
            status_bar->set_text(std::format("Line {}, Column {}", line + 1, col + 1));
        }
    } else {
        status_bar->set_text("No file open");
    }

    main_widget->draw();
    Renderer::instance().flush(size());

    // TODO: Refactor this.
    // if (requested_frames > 0) {
    //     setAutoRedraw(true);
    // } else {
    //     setAutoRedraw(false);
    // }
}

void EditorWindow::on_frame(int64_t ms) {
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

    // spdlog::info("{} {}", vel_x, vel_y);
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
        set_auto_redraw(false);
    }

    // last_ms = ms;
}

// TODO: Verify that resize is always called on all platforms when the window is created.
void EditorWindow::layout() {
    main_widget->set_size(size());
    main_widget->layout();
    // side_bar->setMinimumWidth(100);
    // side_bar->setMaximumWidth(size.width - 100);

    // Manually redraw just to be safe. Windows does not automatically redraw when the window size
    // shrinks.
    redraw();
}

void EditorWindow::perform_scroll(const Point& mouse_pos, const Delta& delta) {
    main_widget->mouse_position_changed(mouse_pos);
    main_widget->perform_scroll(mouse_pos, delta);

    // https://zed.dev/blog/120fps
    requested_frames = frames_per_second();
    set_auto_redraw(true);
    redraw();
}

void EditorWindow::left_mouse_down(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    dragged_widget = main_widget->widget_at(mouse_pos);
    if (dragged_widget) {
        if (dragged_widget->can_be_focused()) {
            focused_widget = dragged_widget;
        }

        dragged_widget->left_mouse_down(mouse_pos, modifiers, click_type);
        main_widget->layout();
        // TODO: See if we should call `updateCursorStyle()` here.
        // TODO: Not all widgets should initiate a drag.
        redraw();
    }

    // TODO: Clean this up.
    // if (dragged_widget) {
    //     spdlog::info("dragged widget: {}", dragged_widget->className());
    // } else {
    //     spdlog::info("no dragged widget");
    // }
    // if (focused_widget) {
    //     spdlog::info("focused widget: {}", focused_widget->className());
    // } else {
    //     spdlog::info("no focused widget");
    // }
}

void EditorWindow::left_mouse_drag(const Point& mouse_pos,
                                   ModifierKey modifiers,
                                   ClickType click_type) {
    if (dragged_widget) {
        dragged_widget->left_mouse_drag(mouse_pos, modifiers, click_type);
        main_widget->layout();
        // TODO: See if we should call `updateCursorStyle()` here.
        // TODO: Not all widgets should initiate a drag.
        redraw();
    }
}

void EditorWindow::left_mouse_up(const Point& mouse_pos) {
    if (dragged_widget) {
        dragged_widget->left_mouse_up(mouse_pos);
        redraw();
    }
    dragged_widget = nullptr;
    update_cursor_style(mouse_pos);
}

void EditorWindow::right_mouse_down(const Point& mouse_pos,
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
    //         spdlog::info("Selected menu index = {}", selected_index.value());
    //     } else {
    //         spdlog::info("Menu was closed without a selection.");
    //     }
    // }
}

// This represents the mouse moving *without* being a click+drag.
bool EditorWindow::mouse_position_changed(const std::optional<Point>& mouse_pos) {
    update_cursor_style(mouse_pos);

    if (main_widget->mouse_position_changed(mouse_pos)) {
        redraw();
        return true;
    } else {
        return false;
    }
}

// TODO: At this point, this is GTK-specific. Make the deceleration feel more natural.
void EditorWindow::on_scroll_decelerate(const Point& mouse_pos, const Delta& delta) {
    vel_x = delta.dx;
    vel_y = delta.dy;
    last_mouse_pos = mouse_pos;

    vel_x /= 16;
    vel_y /= 16;

    set_auto_redraw(true);
    redraw();
}

bool EditorWindow::on_key_down(Key key, ModifierKey modifiers) {
    // spdlog::info("key = {}, modifiers = {}", key, modifiers);

    bool handled = false;
    if (key == Key::kJ && modifiers == kPrimaryModifier) {
        editor_widget->prev_index();
        handled = true;
    } else if (key == Key::kK && modifiers == kPrimaryModifier) {
        editor_widget->next_index();
        handled = true;
    } else if (key == Key::k1 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(0);
        handled = true;
    } else if (key == Key::k2 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(1);
        handled = true;
    } else if (key == Key::k3 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(2);
        handled = true;
    } else if (key == Key::k4 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(3);
        handled = true;
    } else if (key == Key::k5 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(4);
        handled = true;
    } else if (key == Key::k6 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(5);
        handled = true;
    } else if (key == Key::k7 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(6);
        handled = true;
    } else if (key == Key::k8 && modifiers == kPrimaryModifier) {
        editor_widget->set_index(7);
        handled = true;
    } else if (key == Key::k9 && modifiers == kPrimaryModifier) {
        editor_widget->last_index();
        handled = true;
    } else if (key == Key::kA && modifiers == kPrimaryModifier) {
        editor_widget->current_widget()->select_all();
        handled = true;
    } else if (key == Key::kN && modifiers == kPrimaryModifier) {
        auto p = base::Profiler{"Add new tab (modifier key)"};
        // editor_widget->add_tab("sample_text.txt", kSampleText * 50 + kLongLine);
        editor_widget->add_tab("untitled", "");
        handled = true;
    } else if (key == Key::kN && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        parent.create_window();
        handled = true;
    } else if (key == Key::kW && modifiers == kPrimaryModifier) {
        // If we are dragging on the current TextViewWidget, invalidate the pointer to prevent a
        // use-after-free crash.
        if (dragged_widget == editor_widget->current_widget()) {
            dragged_widget = nullptr;
        }

        editor_widget->remove_tab(editor_widget->get_current_index());
        handled = true;
    } else if (key == Key::kW && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        // Immediately exit this function and don't let any other methods get called! This window's
        // smart pointer will be deallocated after `close()`.
        close();
        return true;
    } else if (key == Key::kC && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->current_widget();
        parent.set_clipboard_string(text_view->get_selection_text());
        handled = true;
    } else if (key == Key::kV && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->current_widget();
        text_view->insert_text(parent.get_clipboard_string());
        handled = true;
    } else if (key == Key::kX && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->current_widget();
        parent.set_clipboard_string(text_view->get_selection_text());
        text_view->left_delete();
        handled = true;
    } else if (key == Key::kO && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        auto path = open_file_picker();
        if (path) {
            editor_widget->open_file(*path);
            editor_widget->last_index();
            handled = true;
        }
    } else if (key == Key::kZ && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->current_widget();
        text_view->undo();
        handled = true;
    } else if (key == Key::kZ && modifiers == (kPrimaryModifier | ModifierKey::kShift)) {
        auto* text_view = editor_widget->current_widget();
        text_view->redo();
        handled = true;
    } else if (key == Key::kBackspace && modifiers == ModifierKey::kNone) {
        auto* text_view = editor_widget->current_widget();
        text_view->left_delete();
        handled = true;
    } else if (key == Key::kEnter && modifiers == ModifierKey::kNone) {
        auto* text_view = editor_widget->current_widget();
        text_view->insert_text("\n");
        // focused_widget->insertText("\n");
        handled = true;
    } else if (key == Key::kTab && modifiers == ModifierKey::kNone) {
        auto* text_view = editor_widget->current_widget();
        // TODO: Don't hard code this.
        text_view->insert_text("    ");
        handled = true;
    } else if (key == Key::kQ && modifiers == kPrimaryModifier) {
        parent.quit();
    } else if (key == Key::kF && modifiers == kPrimaryModifier) {
        auto* text_view = editor_widget->current_widget();
        // TODO: Don't hard code this.
        text_view->find("needle");
        handled = true;
    }

    // TODO: Remove this.
    // if (key == Key::kU && modifiers == kPrimaryModifier) {
    //     perform_scroll({500, 500}, {0, -1});
    //     handled = true;
    // }
    // if (key == Key::kI && modifiers == kPrimaryModifier) {
    //     perform_scroll({500, 500}, {0, 1});
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

        int new_font_size = std::max(metrics.font_size - 1, 8);
        spdlog::info("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resize_font(parent.main_font_id, new_font_size);

        editor_widget->update_font(parent.main_font_id);

        handled = true;
    } else if (key == Key::kEqual && modifiers == kPrimaryModifier) {
        auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.metrics(parent.main_font_id);

        int new_font_size = std::min(metrics.font_size + 1, 128);
        spdlog::info("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resize_font(parent.main_font_id, new_font_size);

        editor_widget->update_font(parent.main_font_id);

        handled = true;
    } else if (key == Key::k0 && modifiers == kPrimaryModifier) {
        auto& font_rasterizer = font::FontRasterizer::instance();
        const auto& metrics = font_rasterizer.metrics(parent.main_font_id);

        int new_font_size = parent.kMainFontSize;
        spdlog::info("font size = {}", new_font_size);
        parent.main_font_id = font_rasterizer.resize_font(parent.main_font_id, new_font_size);

        editor_widget->update_font(parent.main_font_id);

        handled = true;
    }

    if (handled) {
        redraw();
    }
    return handled;
}

void EditorWindow::on_insert_text(std::string_view text) {
    if (auto widget = editor_widget->current_widget()) {
        widget->insert_text(text);
        redraw();
    }

    if (focused_widget) {
        focused_widget->insert_text(text);
        redraw();
        // auto editor_widget = static_cast<EditorWidget*>(focused_widget);
        // if (auto widget = editor_widget->currentWidget()) {
        //     widget->insertText(text);
        //     redraw();
        // }
    }
}

void EditorWindow::on_action(Action action, bool extend) {
    auto p = base::Profiler{"EditorWindow::onAction()"};

    bool handled = true;
    auto* text_view = editor_widget->current_widget();
    switch (action) {
    case Action::kMoveForwardByCharacters:
        text_view->move(gui::MoveBy::kCharacters, true, extend);
        break;
    case Action::kMoveBackwardByCharacters:
        text_view->move(gui::MoveBy::kCharacters, false, extend);
        break;
    case Action::kMoveForwardByLines:
        text_view->move(gui::MoveBy::kLines, true, extend);
        break;
    case Action::kMoveBackwardByLines:
        text_view->move(gui::MoveBy::kLines, false, extend);
        break;
    case Action::kMoveForwardByWords:
        text_view->move(gui::MoveBy::kWords, true, extend);
        break;
    case Action::kMoveBackwardByWords:
        text_view->move(gui::MoveBy::kWords, false, extend);
        break;
    case Action::kMoveToBOL:
        text_view->move_to(gui::MoveTo::kBOL, extend);
        break;
    case Action::kMoveToEOL:
        text_view->move_to(gui::MoveTo::kEOL, extend);
        break;
    case Action::kMoveToHardBOL:
        text_view->move_to(gui::MoveTo::kHardBOL, extend);
        break;
    case Action::kMoveToHardEOL:
        text_view->move_to(gui::MoveTo::kHardEOL, extend);
        break;
    case Action::kMoveToBOF:
        text_view->move_to(gui::MoveTo::kBOF, extend);
        break;
    case Action::kMoveToEOF:
        text_view->move_to(gui::MoveTo::kEOF, extend);
        break;
    case Action::kInsertNewline:
        text_view->insert_text("\n");
        // focused_widget->insertText("\n");
        break;
    case Action::kInsertNewlineIgnoringFieldEditor:
        text_view->insert_text("\n");
        // focused_widget->insertText("\n");
        // This command is sent as the first part of the `ctrl+o` keybind. We shouldn't redraw.
        return;
    case Action::kInsertTab:
        text_view->insert_text("    ");
        break;
    case Action::kLeftDelete:
        text_view->left_delete();
        break;
    case Action::kRightDelete:
        text_view->right_delete();
        break;
    case Action::kDeleteWordForward:
        text_view->delete_word(true);
        break;
    case Action::kDeleteWordBackward:
        text_view->delete_word(false);
        break;
    default:
        handled = false;
    }

    if (handled) {
        redraw();
    }
}

void EditorWindow::on_app_action(AppAction action) {
    switch (action) {
    case AppAction::kNewFile:
        editor_widget->add_tab("untitled", "");
        redraw();
        break;
    case AppAction::kNewWindow:
        parent.create_window();
        break;
    }
}

void EditorWindow::on_close() { parent.destroy_window(wid); }

void EditorWindow::update_cursor_style(const std::optional<Point>& mouse_pos) {
    // Case 1: Dragging operation in progress.
    if (dragged_widget) {
        set_cursor_style(dragged_widget->cursor_style());
    }
    // Case 2: Mouse position is within window.
    else if (mouse_pos) {
        if (auto hovered_widget = main_widget->widget_at(mouse_pos.value())) {
            // spdlog::info("{}", hovered_widget->className());
            set_cursor_style(hovered_widget->cursor_style());
        } else {
            // spdlog::info("No widget hovered");
            set_cursor_style(CursorStyle::kArrow);
        }
    }
    // Case 3: Mouse position is outside of window.
    else {
        set_cursor_style(CursorStyle::kArrow);
    }
}

}  // namespace gui
