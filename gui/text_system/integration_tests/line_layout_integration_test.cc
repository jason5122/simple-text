#include "base/buffer/piece_tree.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/line_layout_cache.h"
#include "gui/text_system/caret.h"
#include <gtest/gtest.h>

namespace {

const std::string kMainFontFace = "Source Code Pro";
constexpr int kMainFontSize = 12 * 2;

}  // namespace

namespace gui {

TEST(LineLayoutIntegrationTest, CaretMovement1) {
    auto& font_rasterizer = font::FontRasterizer::instance();
    size_t font_id = font_rasterizer.addFont(kMainFontFace, kMainFontSize);

    base::PieceTree tree{"Hi 🙂🙂 Hello world!\nThis is a new line."};
    LineLayoutCache line_layout_cache;
    // Caret caret{};

    // Cache all piece tree lines.
    for (size_t line = 0; line < tree.line_count(); ++line) {
        std::string line_str = tree.get_line_content_with_newline(line);
        if (!line_str.empty() && line_str.back() == '\n') {
            line_str.back() = ' ';
        }

        const auto& line_layout = line_layout_cache.get(font_id, line_str);
        EXPECT_GT(line_layout.width, 0);
    }
}

}  // namespace gui
