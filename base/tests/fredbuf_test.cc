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

    LineRange line_range = tree.get_line_range(Line{2});
    size_t line_start_offset =
        static_cast<typename std::underlying_type<CharOffset>::type>(line_range.first);
    std::cout << line_start_offset << '\n';

    tree.remove(CharOffset{line_start_offset + 5}, Length{1});
    tree.insert(CharOffset{line_start_offset + 5}, "B");

    std::cout << text << '\n';

    size_t line_count =
        static_cast<typename std::underlying_type<CharOffset>::type>(tree.line_count());
    for (size_t line_index = 1; line_index < line_count; line_index++) {
        std::string buf;
        tree.get_line_content(&buf, Line{line_index});
        std::cout << buf << '\n';
    }
}
