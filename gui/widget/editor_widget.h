#pragma once

#include "gui/widget/container/multi_view_widget.h"
#include "gui/widget/container/vertical_layout_widget.h"
#include "gui/widget/tab_bar_widget.h"
#include "gui/widget/text_edit_widget.h"

namespace gui {

class EditorWidget : public VerticalLayoutWidget {
public:
    EditorWidget(size_t main_font_id, size_t ui_font_size, size_t panel_close_image_id);

    TextEditWidget* currentWidget() const;
    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void lastIndex();
    size_t getCurrentIndex();
    void addTab(std::string_view tab_name, std::string_view text);
    void removeTab(size_t index);
    void openFile(std::string_view path);

    constexpr std::string_view className() const final override {
        return "EditorWidget";
    }

private:
    static constexpr int kTabBarHeight = 29 * 2;

    // Light.
    // static constexpr Rgba kTabBarColor{190, 190, 190, 255};
    // static constexpr Rgba kTextViewColor{253, 253, 253, 255};
    // Dark.
    static constexpr Rgba kTabBarColor{79, 86, 94, 255};
    static constexpr Rgba kTextViewColor{48, 56, 65, 255};

    size_t main_font_id;

    // These cache unique_ptrs. These are guaranteed to be non-null since they are owned by
    // MultiViewWidget.
    MultiViewWidget<TextEditWidget>* multi_view;
    TabBarWidget* tab_bar;
};

}  // namespace gui
