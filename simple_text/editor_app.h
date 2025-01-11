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
    static constexpr int kMainFontSize = 24;
    static constexpr int kUIFontSizeSmall = 11;
    static constexpr int kUIFontSizeRegular = 12;
    const std::string kMainFontFace = "Source Code Pro";
    // const std::string kMainFontFace = "Menlo";
#elif BUILDFLAG(IS_WIN)
    constexpr int kMainFontSize = 11 * 2;
    constexpr int kUIFontSizeSmall = 8 * 2;
    const std::string kMainFontFace = "Source Code Pro";
// const std::string kMainFontFace = "Consolas";
// const std::string kMainFontFace = "Cascadia Code";
#elif BUILDFLAG(IS_LINUX)
    constexpr int kMainFontSize = 12 * 2;
    constexpr int kUIFontSizeSmall = 11 * 2;
    const std::string kMainFontFace = "Source Code Pro";
    // const std::string kMainFontFace = "Monospace";
#endif

    size_t main_font_id;
    size_t ui_font_small_id;
    size_t ui_font_regular_id;

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
