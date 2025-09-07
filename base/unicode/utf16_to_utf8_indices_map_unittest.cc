#include "base/unicode/utf16_to_utf8_indices_map.h"
#include <gtest/gtest.h>

namespace base {

TEST(UTF16ToUTF8IndicesMapTest, MapIndex) {
    UTF16ToUTF8IndicesMap m;
    ASSERT_TRUE(m.set_utf8("Foo©bar𝌆baz☃qux"));
    ASSERT_EQ(m.size(), 16);

    // "foo"
    EXPECT_EQ(m[0], 0);
    EXPECT_EQ(m[1], 1);
    EXPECT_EQ(m[2], 2);
    // "©"
    EXPECT_EQ(m[3], 3);
    // "bar"
    EXPECT_EQ(m[4], 5);
    EXPECT_EQ(m[5], 6);
    EXPECT_EQ(m[6], 7);
    // "𝌆"
    EXPECT_EQ(m[7], 8);
    // "baz"
    EXPECT_EQ(m[9], 12);
    EXPECT_EQ(m[10], 13);
    EXPECT_EQ(m[11], 14);
    // "☃"
    EXPECT_EQ(m[12], 15);
    // "qux"
    EXPECT_EQ(m[13], 18);
    EXPECT_EQ(m[14], 19);
    EXPECT_EQ(m[15], 20);
}

TEST(UTF16ToUTF8IndicesMapTest, MapIndex2) {
    UTF16ToUTF8IndicesMap m;
    // clang-format off
    ASSERT_TRUE(m.set_utf8("A" "\xF0\x9F\x98\x80" "B"));
    // clang-format on
    ASSERT_EQ(m.size(), 4);

    EXPECT_EQ(m[0], 0);
    EXPECT_EQ(m[1], 1);
    EXPECT_EQ(m[2], 1);
    EXPECT_EQ(m[3], 5);
}

TEST(UTF16ToUTF8IndicesMapTest, InvalidUTF8) {
    constexpr auto kInvalids = std::to_array<std::string_view>({
        "\x80",              // lone continuation
        "\xE2\x28\xA1",      // wrong continuation
        "\xC2",              // truncated 2-byte
        "\xE2\x82",          // truncated 3-byte
        "\xF0\x9F\x92",      // truncated 4-byte
        "\xC0\x80",          // overlong via illegal lead
        "\xF5\x80\x80\x80",  // > U+10FFFF
        "\xFF",              // illegal byte
    });

    for (std::string_view s : kInvalids) {
        UTF16ToUTF8IndicesMap m;
        EXPECT_FALSE(m.set_utf8(s));
    }
}

}  // namespace base
