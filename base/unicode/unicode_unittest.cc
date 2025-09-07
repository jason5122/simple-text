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

TEST(UnicodeTest, NextUTF8BasicBMP) {
    constexpr auto kAscii = std::to_array<std::string_view>({
        "",
        "hello world",
        "\n\r\t",
        "\x0",
    });

    for (std::string_view s : kAscii) {
        size_t expected_i = 0;
        for (size_t i = 0; i < s.length();) {
            EXPECT_EQ(s[i], next_utf8(s, i));
            EXPECT_EQ(i, ++expected_i);
        }
    }
}

TEST(UnicodeTest, NextUTF8AdvanceIndexAfterInvalid) {
    // clang-format off
    constexpr std::string_view s = "a\x80" "b";
    // clang-format on
    size_t i = 0;
    EXPECT_EQ(next_utf8(s, i), u'a');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(next_utf8(s, i), -1);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(next_utf8(s, i), u'b');
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF8OutOfBounds) {
    constexpr std::string_view s = "abc";
    size_t i = 99;
    EXPECT_EQ(next_utf8(s, i), -1);
    EXPECT_EQ(i, 99);
}

TEST(UnicodeTest, NextUTF16BasicBMP) {
    constexpr auto kAscii = std::to_array<std::u16string_view>({
        u"",
        u"hello world",
        u"\n\r\t",
        u"\x0",
    });

    for (std::u16string_view s : kAscii) {
        size_t expected_i = 0;
        for (size_t i = 0; i < s.length();) {
            EXPECT_EQ(s[i], next_utf16(s, i));
            EXPECT_EQ(i, ++expected_i);
        }
    }
}

TEST(UnicodeTest, NextUTF16SurrogatePair) {
    // 😀 U+1F600 as UTF-16 surrogate pair
    constexpr std::u16string_view s = u"\U0001F600";
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), 0x1F600);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16Mixed) {
    // "A😀B"
    constexpr std::u16string_view s = u"A\U0001F600B";
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), u'A');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(next_utf16(s, i), 0x1F600);
    EXPECT_EQ(i, 3);
    EXPECT_EQ(next_utf16(s, i), u'B');
    EXPECT_EQ(i, 4);
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16VariationSelector16) {
    // ☂️  = U+2602 U+FE0F (two code points, both BMP)
    constexpr std::u16string_view s = u"\u2602\uFE0F";
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), 0x2602);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(next_utf16(s, i), 0xFE0F);
    EXPECT_EQ(i, 2);
}

TEST(UnicodeTest, NextUTF16InvalidUnpairedHigh) {
    constexpr std::u16string_view s = u"\xD800";  // Lone high surrogate.
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16InvalidUnpairedLow) {
    constexpr std::u16string_view s = u"\xDC00";  // Lone low surrogate.
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16TwoHighs) {
    constexpr std::u16string_view s = u"\xD800\xD800";
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16AdvanceIndexAfterInvalid) {
    // clang-format off
    constexpr std::u16string_view s = u"ab\xD800" "cd";
    // clang-format on
    size_t i = 0;
    EXPECT_EQ(next_utf16(s, i), u'a');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(next_utf16(s, i), u'b');
    EXPECT_EQ(i, 2);
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, 3);
    EXPECT_EQ(next_utf16(s, i), u'c');
    EXPECT_EQ(i, 4);
    EXPECT_EQ(next_utf16(s, i), u'd');
    EXPECT_EQ(i, s.length());
}

TEST(UnicodeTest, NextUTF16OutOfBounds) {
    constexpr std::u16string_view s = u"abc";
    size_t i = 99;
    EXPECT_EQ(next_utf16(s, i), -1);
    EXPECT_EQ(i, 99);
}

TEST(UnicodeTest, CodepointToUTF8NoOutput) {
    EXPECT_EQ(codepoint_to_utf8(U'A'), 1);
    EXPECT_EQ(codepoint_to_utf8(U'©'), 2);
    EXPECT_EQ(codepoint_to_utf8(U'€'), 3);
    EXPECT_EQ(codepoint_to_utf8(U'😀'), 4);
    EXPECT_EQ(codepoint_to_utf8(0xD800), -1);
}

TEST(UnicodeTest, CodepointToUTF8WithOutput) {
    using namespace std::literals;

    char out[4] = {};
    EXPECT_EQ(codepoint_to_utf8(U'A', out), 1);
    EXPECT_EQ(std::string_view(out, 1), "\x41"sv);

    EXPECT_EQ(codepoint_to_utf8(U'©', out), 2);
    EXPECT_EQ(std::string_view(out, 2), "\xC2\xA9"sv);

    EXPECT_EQ(codepoint_to_utf8(U'€', out), 3);
    EXPECT_EQ(std::string_view(out, 3), "\xE2\x82\xAC"sv);

    EXPECT_EQ(codepoint_to_utf8(U'😀', out), 4);
    EXPECT_EQ(std::string_view(out, 4), "\xF0\x9F\x98\x80"sv);

    EXPECT_EQ(codepoint_to_utf8(0xD800, out), -1);
}

TEST(UnicodeTest, CodepointToUTF16NoOutput) {
    EXPECT_EQ(codepoint_to_utf16(U'A'), 1);
    EXPECT_EQ(codepoint_to_utf16(U'©'), 1);
    EXPECT_EQ(codepoint_to_utf16(U'€'), 1);
    EXPECT_EQ(codepoint_to_utf16(U'😀'), 2);
    EXPECT_EQ(codepoint_to_utf16(0xD800), -1);
}

TEST(UnicodeTest, CodepointToUTF16WithOutput) {
    using namespace std::literals;

    uint16_t out[4] = {};

    // 'A' -> 0041
    EXPECT_EQ(codepoint_to_utf16(U'A', out), 1);
    EXPECT_EQ(out[0], 0x0041);

    // © U+00A9 -> 00A9
    EXPECT_EQ(codepoint_to_utf16(U'©', out), 1);
    EXPECT_EQ(out[0], 0x00A9);

    // € U+20AC -> 20AC
    EXPECT_EQ(codepoint_to_utf16(U'€', out), 1);
    EXPECT_EQ(out[0], 0x20AC);

    // 😀 U+1F600 -> D83D DE00
    EXPECT_EQ(codepoint_to_utf16(U'😀', out), 2);
    EXPECT_EQ(out[0], 0xD83D);
    EXPECT_EQ(out[1], 0xDE00);

    EXPECT_EQ(codepoint_to_utf16(0xD800, out), -1);
}

TEST(UnicodeTest, UTF8ToUTF16Length) {
    struct Case {
        std::string_view s;
        int expected_units;
    };
    // Note: counts are UTF-16 *code units*, not code points.
    constexpr auto kCases = std::to_array<Case>({
        {"", 0},                          // empty
        {"hello", 5},                     // ASCII
        {"\xC2\xA9", 1},                  // © U+00A9 (BMP)
        {"\xE2\x82\xAC", 1},              // € U+20AC (BMP)
        {"\xF0\x9F\x98\x80", 2},          // 😀 U+1F600 (astral)
        {"\xE2\x98\x82\xEF\xB8\x8F", 2},  // ☂️ = U+2602, U+FE0F (both BMP)
        {"A😀B", 4},                      // A😀B -> 1 + 2 + 1
        {"😀😀", 4},                      // 😀😀 (2 + 2)
        {"e\xCC\x81", 2},                 // e + COMBINING ACUTE (BMP + BMP)
        {"\xF4\x8F\xBF\xBF", 2},          // U+10FFFF (max scalar, astral)
    });

    for (const auto& c : kCases) {
        SCOPED_TRACE(::testing::Message() << "bytes=" << c.s.size());
        EXPECT_EQ(utf8_to_utf16_length(c.s), c.expected_units);
    }

    EXPECT_EQ(utf8_to_utf16_length(""), 0);
    EXPECT_EQ(utf8_to_utf16_length("😀"), 2);
}

TEST(Utf8ToUtf16Length, UTF8ToUTF16LengthInvalid) {
    // Each case has exactly one invalid sequence.
    constexpr auto kInvalids = std::to_array<std::string_view>({
        "\x80",              // lone continuation
        "\xE2\x28\xA1",      // wrong continuation inside 3-byte
        "\xC2",              // truncated 2-byte
        "\xE2\x82",          // truncated 3-byte
        "\xF0\x9F\x92",      // truncated 4-byte
        "\xC0\x80",          // overlong encoding of NUL (illegal)
        "\xED\xA0\x80",      // UTF-8 for U+D800 (surrogate; invalid in UTF-8)
        "\xF5\x80\x80\x80",  // lead > U+10FFFF
        "\xFF",              // illegal byte
    });
    for (std::string_view s : kInvalids) {
        EXPECT_EQ(utf8_to_utf16_length(s), -1);
    }
}

TEST(Utf8ToUtf16Length, UTF8ToUTF16LengthLongMixedSequence) {
    // "Foo©bar😀baz☃️qux" -> count BMP vs astral:
    // Foo(3) + ©(1) + bar(3) + 😀(2) + baz(3) + ☃️(U+2603 U+FE0F -> 2) + qux(3) = 17
    const std::string_view s = "Foo"
                               "\xC2\xA9"
                               "bar"
                               "\xF0\x9F\x98\x80"
                               "baz"
                               "\xE2\x98\x83\xEF\xB8\x8F"
                               "qux";
    EXPECT_EQ(utf8_to_utf16_length(s), 17);
}

}  // namespace base
