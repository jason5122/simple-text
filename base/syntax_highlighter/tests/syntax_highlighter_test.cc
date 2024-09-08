#include "base/buffer/piece_table.h"
#include "base/syntax_highlighter/syntax_highlighter.h"
#include <gtest/gtest.h>

extern "C" TSLanguage* tree_sitter_json();

namespace base {

TEST(SyntaxHighlighterTest, Basic) {
    std::string json = R"({
  "x": 10,
})";
    PieceTable table{json};

    SyntaxHighlighter highlighter;

    highlighter.mystery();
    TSInput input = {&table, base::SyntaxHighlighter::read, TSInputEncodingUTF8};
    highlighter.parse(input);

    auto highlights = highlighter.getHighlights(0, table.length());
    for (const auto& [start_byte, end_byte, capture_index] : highlights) {
        auto rgb = highlighter.getColor(capture_index);
        std::cerr << std::format("[{}, {}] {}\n", start_byte, end_byte, capture_index);
        std::cerr << rgb << '\n';
    }
}

}
