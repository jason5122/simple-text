#include "unicode/unicode.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "base/buffer/piece_tree.h"
#include "third_party/uni_algo/include/uni_algo/prop.h"
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
    }
    // https://www.rfc-editor.org/rfc/rfc3629
    // The octet values C0, C1, F5 to FF never appear.
    else if (c >= 0xF5 || (c & 0xFE) == 0xC0) {
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

constexpr inline int32_t left_shift(int32_t value, int32_t shift) {
    return (int32_t)((uint32_t)value << shift);
}

int32_t NextUTF8PieceTree(const base::PieceTree& tree) {
    if (tree.empty()) return -1;

    base::TreeWalker walker{&tree};
    int c = walker.current();
    int hic = c << 24;

    if (!utf8_type_is_valid_leading_byte(utf8_byte_type(c))) {
        return -1;
    }
    if (hic < 0) {
        uint32_t mask = (uint32_t)~0x3F;
        hic = left_shift(hic, 1);
        do {
            walker.next();
            if (walker.exhausted()) {
                return -1;
            }
            // check before reading off end of array.
            uint8_t nextByte = walker.current();
            if (!utf8_byte_is_continuation(nextByte)) {
                return -1;
            }
            c = (c << 6) | (nextByte & 0x3F);
            mask <<= 5;
        } while ((hic = left_shift(hic, 1)) < 0);
        c &= ~mask;
    }
    walker.next();
    std::println("Next codepoint start = {}", walker.offset());
    return c;
}
}  // namespace

TEST(UnicodeTest, PieceTreeTest1) {
    base::PieceTree tree1{"hello world"};
    EXPECT_EQ(CountUTF8PieceTree(tree1), 11);

    base::PieceTree tree2{"⌚..⌛⏩..⏬☂️..☃️"};
    EXPECT_EQ(CountUTF8PieceTree(tree2), 14);

    base::PieceTree tree3{"﷽"};
    EXPECT_EQ(CountUTF8PieceTree(tree3), 1);
}

TEST(UnicodeTest, PieceTreeTest2) {
    EXPECT_TRUE(una::codepoint::is_alphabetic('a'));

    std::string str = "﷽";
    const char* ptr = str.data();
    int32_t unichar = NextUTF8(&ptr, ptr + str.size());
    std::println("unichar = {}", unichar);
    EXPECT_FALSE(una::codepoint::is_alphabetic(unichar));

    base::PieceTree tree{"﷽"};
    unichar = NextUTF8PieceTree(tree);
    std::println("NextUTF8PieceTree unichar = {}", unichar);

    EXPECT_TRUE(una::codepoint::is_whitespace(' '));
    EXPECT_TRUE(una::codepoint::is_whitespace('\n'));
    EXPECT_TRUE(una::codepoint::is_whitespace('\r'));
}

}  // namespace unicode
