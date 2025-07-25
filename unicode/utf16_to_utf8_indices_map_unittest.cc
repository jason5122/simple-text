#include "unicode/utf16_to_utf8_indices_map.h"
#include <gtest/gtest.h>

namespace unicode {

TEST(UTF16ToUTF8IndicesMapTest, MapIndex) {
    std::string str = "Foo©bar𝌆baz☃qux";
    UTF16ToUTF8IndicesMap indices_map;
    indices_map.set_utf8(str.data(), str.length());

    // "foo"
    EXPECT_EQ(indices_map.map_index(0), 0);
    EXPECT_EQ(indices_map.map_index(1), 1);
    EXPECT_EQ(indices_map.map_index(2), 2);
    // "©"
    EXPECT_EQ(indices_map.map_index(3), 3);
    // "bar"
    EXPECT_EQ(indices_map.map_index(4), 5);
    EXPECT_EQ(indices_map.map_index(5), 6);
    EXPECT_EQ(indices_map.map_index(6), 7);
    // "𝌆"
    EXPECT_EQ(indices_map.map_index(7), 8);
    // "baz"
    EXPECT_EQ(indices_map.map_index(9), 12);
    EXPECT_EQ(indices_map.map_index(10), 13);
    EXPECT_EQ(indices_map.map_index(11), 14);
    // "☃"
    EXPECT_EQ(indices_map.map_index(12), 15);
    // "qux"
    EXPECT_EQ(indices_map.map_index(13), 18);
    EXPECT_EQ(indices_map.map_index(14), 19);
    EXPECT_EQ(indices_map.map_index(15), 20);
}

}  // namespace unicode
