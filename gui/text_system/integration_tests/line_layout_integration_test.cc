#include "base/buffer/piece_table.h"
#include "font/font_rasterizer.h"
#include "gui/text_system/caret.h"
#include "gui/text_system/line_layout_cache.h"
#include <gtest/gtest.h>

namespace {

const std::string kMainFontFace = "Source Code Pro";
constexpr int kMainFontSize = 12 * 2;

}

namespace gui {

TEST(LineLayoutIntegrationTest, CaretMovement1) {
    auto& font_rasterizer = font::FontRasterizer::instance();
    size_t font_id = font_rasterizer.addFont(kMainFontFace, kMainFontSize);

    base::PieceTable table{"Hi 🙂🙂 Hello world!\nThis is a new line."};
    LineLayoutCache line_layout_cache{font_id};
    // Caret caret{};

    // Cache all piece table lines.
    for (size_t line = 0; line < table.lineCount(); ++line) {
        std::string line_str = table.line(line);
        if (!line_str.empty() && line_str.back() == '\n') {
            line_str.back() = ' ';
        }

        const auto& line_layout = line_layout_cache.getLineLayout(line_str);
        EXPECT_GT(line_layout.width, 0);
    }
}

}
