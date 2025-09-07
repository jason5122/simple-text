#include "base/unicode/unicode.h"
#include <gtest/gtest.h>

namespace base {

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

TEST(UnicodeTest, CountUTF16) {
    EXPECT_EQ(count_utf16(u""), 0);
    EXPECT_EQ(count_utf16(u"hello world"), 11);
    EXPECT_EQ(count_utf16(u"⌚..⌛⏩..⏬☂️..☃️"), 14);
    EXPECT_EQ(count_utf16(u"﷽"), 1);

    // Surrogate pairs.
    EXPECT_EQ(count_utf16(u"\xD83D\xDE00"), 1);  // 😀
    EXPECT_EQ(count_utf16(u"\xD834\xDD1E"), 1);  // 𝄞
    EXPECT_EQ(count_utf16(u"\xD834\xDF06"), 1);  // 𝌆
}

TEST(UnicodeTest, CountUTF16Invalid) {
    EXPECT_EQ(count_utf16(u"\xD800"), -1);
    EXPECT_EQ(count_utf16(u"\xDC00"), -1);

    // clang-format off
    EXPECT_EQ(count_utf16(u"\xD800" "A"), -1);
    EXPECT_EQ(count_utf16(u"\xDC00" "A"), -1);
    // clang-format on

    EXPECT_EQ(count_utf16(u"\xDC00\xD800"), -1);
    EXPECT_EQ(count_utf16(u"\xD800\xD800"), -1);
    EXPECT_EQ(count_utf16(u"\xDC00\xDC00"), -1);
}

TEST(UnicodeTest, NextUTF8ASCII) {
    std::string utf8 = "hello world";
    size_t i = 0;
    EXPECT_EQ(next_utf8(utf8, i), 0x68);
    EXPECT_EQ(i, 1);
}

// TODO: Add more tests.

}  // namespace base
