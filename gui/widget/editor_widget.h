#pragma once

#include "gui/widget/container/multi_view_widget.h"
#include "gui/widget/container/vertical_layout_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/text_edit_widget.h"

namespace gui {

class EditorWidget : public VerticalLayoutWidget {
public:
    EditorWidget(size_t main_font_id, size_t ui_font_size, size_t panel_close_image_id);

    TextEditWidget* current_widget() const;
    void set_index(size_t index);
    void prev_index();
    void next_index();
    void last_index();
    size_t get_current_index();
    void add_tab(std::string_view tab_name, std::string_view text);
    void remove_tab(size_t index);
    void open_file(std::string_view path);
    // TODO: Refactor this.
    void update_font(size_t font_id);

    constexpr std::string_view class_name() const final override { return "EditorWidget"; }
    constexpr bool can_be_focused() const override { return true; }

private:
    static constexpr int kTabBarHeight = 29 * 2;

    // Light.
    // static constexpr Rgb kTabBarColor{190, 190, 190};
    // static constexpr Rgb kTextViewColor{253, 253, 253};
    // Dark.
    static constexpr Rgb kTabBarColor{79, 86, 94};
    static constexpr Rgb kTextViewColor{48, 56, 65};

    size_t main_font_id;

    // These cache unique_ptrs. These are guaranteed to be non-null since they are owned by
    // MultiViewWidget.
    MultiViewWidget<TextEditWidget>* multi_view;
    TabBarWidget* tab_bar;
};

}  // namespace gui
