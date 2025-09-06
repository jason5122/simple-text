#include "unicode/unicode.h"
#include <gtest/gtest.h>

namespace unicode {

TEST(UnicodeTest, CountUTF8) {
    EXPECT_EQ(count_utf8(""), 0);
    EXPECT_EQ(count_utf8("hello world"), 11);
    EXPECT_EQ(count_utf8("⌚..⌛⏩..⏬☂️..☃️"), 14);
    EXPECT_EQ(count_utf8("﷽"), 1);
}

TEST(UnicodeTest, CountUTF8Invalid) {
    // Lone continuation byte.
    EXPECT_EQ(count_utf8("\x80"), -1);

    // Wrong continuation inside a 3-byte sequence.
    EXPECT_EQ(count_utf8("\xE2\x28\xA1"), -1);

    // Truncated sequences.
    EXPECT_EQ(count_utf8("\xC2"), -1);
    EXPECT_EQ(count_utf8("\xE2\x82"), -1);
    EXPECT_EQ(count_utf8("\xF0\x9F\x92"), -1);

    // Illegal leading bytes.
    EXPECT_EQ(count_utf8("\xC0\x80"), -1);
    EXPECT_EQ(count_utf8("\xC1\xBF"), -1);
    EXPECT_EQ(count_utf8("\xF5\x80\x80\x80"), -1);
    EXPECT_EQ(count_utf8("\xFF"), -1);
}

}  // namespace unicode
