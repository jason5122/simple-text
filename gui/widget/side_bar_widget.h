#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/scrollable_widget.h"
#include <array>

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(int width);

    void draw() override;
    bool mouse_position_changed(const std::optional<Point>& mouse_pos) override;

    void update_max_scroll() override;

    constexpr std::string_view class_name() const final override { return "SideBarWidget"; }

private:
    // static constexpr Rgb kTextColor = {51, 51, 51};     // Light.
    static constexpr Rgb kTextColor = {230, 230, 230};  // Dark.
    // static constexpr Rgb kSideBarColor = {235, 237, 239};  // Light.
    static constexpr Rgb kSideBarColor = {34, 38, 42};  // Dark.
    // static constexpr Rgb kScrollBarColor = {190, 190, 190};  // Light.
    static constexpr Rgb kScrollBarColor = {105, 112, 118};  // Dark.
    static constexpr Rgb kFolderIconColor = {142, 142, 142};
    static constexpr Rgb kHighlightColor = {255, 255, 0};

    // TODO: Tweak these to match Sublime Text.
    static constexpr int kLeftPadding = 15 * 2;
    static constexpr int kTopPadding = 13 * 2;

    static constexpr int kLabelFontSize = 12;
    static constexpr int kFoldersLabelFontSize = 13;
    size_t label_font_id;
    size_t folders_label_font_id;

    static constexpr std::string_view kFoldersLabel = "FOLDERS";
    static constexpr std::array strs = {"simple-text", ".cache", "app", "base", "build", "config"};
    std::optional<size_t> hovered_index = std::nullopt;

    // Draw helpers.
    // TODO: Rename these methods.
    void render_folder_label();
    void render_labels();
    void render_scroll_bars(int line_height, size_t visible_lines);
};

}  // namespace gui
