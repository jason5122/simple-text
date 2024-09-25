#include "base/buffer/piece_table.h"
#include "gui/text_system/caret.h"
#include "gui/text_system/line_layout_cache.h"
#include <gtest/gtest.h>

namespace gui {

TEST(LineLayoutIntegrationTest, CaretMovement1) {
    base::PieceTable table{R"(Hi 🙂🙂 Hello world!
This is a new line.)"};
    // LineLayoutCache line_layout_cache;
    // Caret caret{};

    // Cache all piece table lines.
    for (size_t line = 0; line < table.lineCount(); ++line) {
        std::string line_str = table.line(line);

        // TODO: Decouple this from OpenGL/renderer.
        // std::ignore = line_layout_cache.getLineLayout(line_str);
    }
}

}
