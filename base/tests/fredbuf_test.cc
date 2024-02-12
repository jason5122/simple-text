#include "third_party/fredbuf/fredbuf.h"
#include "util/file_util.h"
#include "gtest/gtest.h"
#include <iostream>
using namespace PieceTree;

TEST(FredbufTest, Basic) {
    std::string text = ReadFile("example.json");

    TreeBuilder builder;
    builder.accept(text);

    Tree tree = builder.create();

    tree.insert(CharOffset{0}, "foo");

    CharOffset line_offset = tree.get_line_range(Line{2}).first;
    tree.remove(line_offset + Length{5}, Length{1});
    tree.insert(line_offset + Length{5}, "B");

    for (size_t line_index = 1; line_index < rep(tree.line_count()); line_index++) {
        std::string buf;
        tree.get_line_content(&buf, Line{line_index});
        std::cout << buf << '\n';
    }
}
