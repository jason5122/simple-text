#pragma once

#include "gui/platform/app.h"
#include "simple_text/editor_window.h"

#include <vector>

// TODO: Remove this.
#include "build/build_config.h"

namespace gui {

class EditorApp : public App {
public:
    void createWindow();
    void destroyWindow(int wid);

    void onLaunch() override;
    void onQuit() override;
    void onAppAction(AppAction action) override;

private:
    friend class EditorWindow;

    // TODO: Properly load this from settings.
#if BUILDFLAG(IS_MAC)
    static constexpr int kMainFontSize = 16;
    static constexpr int kUIFontSizeSmall = 11;
    static constexpr int kUIFontSizeRegular = 12;
    const std::string kMainFontFace = "Source Code Pro";
    // const std::string kMainFontFace = "Menlo";
#elif BUILDFLAG(IS_WIN)
    static constexpr int kMainFontSize = 11;
    static constexpr int kUIFontSizeSmall = 8;
    static constexpr int kUIFontSizeRegular = 9;
    const std::string kMainFontFace = "Source Code Pro";
// const std::string kMainFontFace = "Consolas";
// const std::string kMainFontFace = "Cascadia Code";
#elif BUILDFLAG(IS_LINUX)
    static constexpr int kMainFontSize = 12;
    static constexpr int kUIFontSizeSmall = 11;
    const std::string kMainFontFace = "Source Code Pro";
    // const std::string kMainFontFace = "Monospace";
#endif

    font::FontId main_font_id;
    font::FontId ui_font_small_id;
    font::FontId ui_font_regular_id;

    size_t panel_close_image_id;
    size_t folder_open_image_id;
    size_t icon_regex_image_id;
    size_t icon_case_sensitive_image_id;
    size_t icon_whole_word_image_id;
    size_t icon_wrap_image_id;
    size_t icon_in_selection_id;
    size_t icon_highlight_matches_id;

    std::vector<std::unique_ptr<EditorWindow>> editor_windows;
};

}  // namespace gui
