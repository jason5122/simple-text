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

    highlighter.getHighlights(0, table.length());
    for (size_t i = 0; i < table.length(); ++i) {
        SyntaxHighlighter::Rgb rgb = highlighter.getColor(i);
        std::cerr << rgb << '\n';
    }
}

}
