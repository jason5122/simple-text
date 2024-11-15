#include "unicode/unicode.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "base/buffer/piece_tree.h"
#include "util/std_print.h"

namespace unicode {

TEST(UnicodeTest, CountUTF8Test) {
    std::string str1 = "hello world";
    EXPECT_EQ(CountUTF8(str1.data(), str1.length()), 11);

    std::string str2 = "⌚..⌛⏩..⏬☂️..☃️";
    EXPECT_EQ(CountUTF8(str2.data(), str2.length()), 14);

    std::string str3 = "﷽";
    EXPECT_EQ(CountUTF8(str3.data(), str3.length()), 1);
}

namespace {
int utf8_byte_type(uint8_t c) {
    if (c < 0x80) {
        return 1;
    } else if (c < 0xC0) {
        return 0;
    } else if (c >= 0xF5 || (c & 0xFE) == 0xC0) {  // "octet values c0, c1, f5 to ff never appear"
        return -1;
    } else {
        int value = (((0xe5 << 24) >> ((unsigned)c >> 4 << 1)) & 3) + 1;
        assert(2 <= value && value <= 4);
        return value;
    }
}
bool utf8_type_is_valid_leading_byte(int type) {
    return type > 0;
}

bool utf8_byte_is_continuation(uint8_t c) {
    return utf8_byte_type(c) == 0;
}

int CountUTF8PieceTree(const base::PieceTree& tree) {
    base::TreeWalker walker{&tree};
    int count = 0;
    while (!walker.exhausted()) {
        int type = utf8_byte_type(walker.current());
        if (!utf8_type_is_valid_leading_byte(type)) {
            // Sequence extends beyond end.
            std::println("Error: Sequence extends beyond end.");
            return -1;
        }
        while (type-- > 1) {
            walker.next();
            if (!utf8_byte_is_continuation(walker.current())) {
                std::println("Error: <TODO: Name this error.>");
                return -1;
            }
        }
        walker.next();
        ++count;
    }
    return count;
}
}  // namespace

TEST(UnicodeTest, PieceTreeTest) {
    base::PieceTree tree1{"hello world"};
    EXPECT_EQ(CountUTF8PieceTree(tree1), 11);

    base::PieceTree tree2{"⌚..⌛⏩..⏬☂️..☃️"};
    EXPECT_EQ(CountUTF8PieceTree(tree2), 14);

    base::PieceTree tree3{"﷽"};
    EXPECT_EQ(CountUTF8PieceTree(tree3), 1);
}

}  // namespace unicode
