#include "base/unicode/utf16_to_utf8_indices_map.h"
#include <gtest/gtest.h>

namespace base {

TEST(UTF16ToUTF8IndicesMapTest, MapIndex) {
    UTF16ToUTF8IndicesMap indices_map;
    indices_map.set_utf8("Foo©bar𝌆baz☃qux");

    // "foo"
    EXPECT_EQ(indices_map[0], 0);
    EXPECT_EQ(indices_map[1], 1);
    EXPECT_EQ(indices_map[2], 2);
    // "©"
    EXPECT_EQ(indices_map[3], 3);
    // "bar"
    EXPECT_EQ(indices_map[4], 5);
    EXPECT_EQ(indices_map[5], 6);
    EXPECT_EQ(indices_map[6], 7);
    // "𝌆"
    EXPECT_EQ(indices_map[7], 8);
    // "baz"
    EXPECT_EQ(indices_map[9], 12);
    EXPECT_EQ(indices_map[10], 13);
    EXPECT_EQ(indices_map[11], 14);
    // "☃"
    EXPECT_EQ(indices_map[12], 15);
    // "qux"
    EXPECT_EQ(indices_map[13], 18);
    EXPECT_EQ(indices_map[14], 19);
    EXPECT_EQ(indices_map[15], 20);
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
