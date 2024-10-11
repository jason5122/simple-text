#include "base/buffer/piece_table.h"
#include "base/syntax_highlighter/syntax_highlighter.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

extern "C" TSLanguage* tree_sitter_json();

namespace base {

TEST(SyntaxHighlighterTest, Basic) {
    std::string json = R"({
  "x": 10,
})";
    PieceTable table{json};

    SyntaxHighlighter highlighter;
    highlighter.setJsonLanguage();
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});

    size_t i = 2;
    std::string str = "  \"y\": 42,\n";
    table.insert(i, str);
    highlighter.edit(i, i, i + str.length());
    highlighter.parse({&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8});

    auto highlights = highlighter.getHighlights(0, table.lineCount());
    for (const auto& [start_byte, end_byte, capture_index] : highlights) {
        auto rgb = highlighter.getColor(capture_index);

        // TODO: Replace this with std::format support.
        std::println("{} {} {}", start_byte, end_byte, capture_index);
        std::println("{}", rgb);
    }
}

}
