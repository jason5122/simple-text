#include "base/strings/string_util.h"
#include <gtest/gtest.h>

namespace base {

namespace {

using TestFunction = bool (*)(std::string_view str);

// Helper used to test IsStringUTF8[AllowingNoncharacters].
void TestStructurallyValidUtf8(TestFunction fn) {
    EXPECT_TRUE(fn(""));
    EXPECT_TRUE(fn("abc"));
    EXPECT_TRUE(fn("\xC2\x81"));
    EXPECT_TRUE(fn("\xE1\x80\xBF"));
    EXPECT_TRUE(fn("\xF1\x80\xA0\xBF"));
    EXPECT_TRUE(fn("\xF1\x80\xA0\xBF"));
    EXPECT_TRUE(fn("a\xC2\x81\xE1\x80\xBF\xF1\x80\xA0\xBF"));

    // U+FEFF used as UTF-8 BOM.
    // clang-format off
    EXPECT_TRUE(fn("\xEF\xBB\xBF" "abc"));
    // clang-format on

    // Embedded nulls in canonical UTF-8 representation.
    using std::string_literals::operator""s;
    const std::string kEmbeddedNull = "embedded\0null"s;
    EXPECT_TRUE(fn(kEmbeddedNull));
}

// Helper used to test IsStringUTF8[AllowingNoncharacters].
void TestStructurallyInvalidUtf8(TestFunction fn) {
    // Invalid encoding of U+1FFFE (0x8F instead of 0x9F)
    EXPECT_FALSE(fn("\xF0\x8F\xBF\xBE"));

    // Surrogate code points
    EXPECT_FALSE(fn("\xED\xA0\x80\xED\xBF\xBF"));
    EXPECT_FALSE(fn("\xED\xA0\x8F"));
    EXPECT_FALSE(fn("\xED\xBF\xBF"));

    // Overlong sequences
    EXPECT_FALSE(fn("\xC0\x80"));                  // U+0000
    EXPECT_FALSE(fn("\xC1\x80\xC1\x81"));          // "AB"
    EXPECT_FALSE(fn("\xE0\x80\x80"));              // U+0000
    EXPECT_FALSE(fn("\xE0\x82\x80"));              // U+0080
    EXPECT_FALSE(fn("\xE0\x9F\xBF"));              // U+07FF
    EXPECT_FALSE(fn("\xF0\x80\x80\x8D"));          // U+000D
    EXPECT_FALSE(fn("\xF0\x80\x82\x91"));          // U+0091
    EXPECT_FALSE(fn("\xF0\x80\xA0\x80"));          // U+0800
    EXPECT_FALSE(fn("\xF0\x8F\xBB\xBF"));          // U+FEFF (BOM)
    EXPECT_FALSE(fn("\xF8\x80\x80\x80\xBF"));      // U+003F
    EXPECT_FALSE(fn("\xFC\x80\x80\x80\xA0\xA5"));  // U+00A5

    // Beyond U+10FFFF (the upper limit of Unicode codespace)
    EXPECT_FALSE(fn("\xF4\x90\x80\x80"));          // U+110000
    EXPECT_FALSE(fn("\xF8\xA0\xBF\x80\xBF"));      // 5 bytes
    EXPECT_FALSE(fn("\xFC\x9C\xBF\x80\xBF\x80"));  // 6 bytes

    // BOM in UTF-16(BE|LE)
    EXPECT_FALSE(fn("\xFE\xFF"));
    EXPECT_FALSE(fn("\xFF\xFE"));

    // Strings in legacy encodings. We can certainly make up strings
    // in a legacy encoding that are valid in UTF-8, but in real data,
    // most of them are invalid as UTF-8.

    // cafe with U+00E9 in ISO-8859-1
    EXPECT_FALSE(fn("caf\xE9"));
    // U+AC00, U+AC001 in EUC-KR
    EXPECT_FALSE(fn("\xB0\xA1\xB0\xA2"));
    // U+4F60 U+597D in Big5
    EXPECT_FALSE(fn("\xA7\x41\xA6\x6E"));
    // "abc" with U+201[CD] in windows-125[0-8]
    // clang-format off
    EXPECT_FALSE(fn("\x93" "abc\x94"));
    // clang-format on
    // U+0639 U+064E U+0644 U+064E in ISO-8859-6
    EXPECT_FALSE(fn("\xD9\xEE\xE4\xEE"));
    // U+03B3 U+03B5 U+03B9 U+03AC in ISO-8859-7
    EXPECT_FALSE(fn("\xE3\xE5\xE9\xDC"));

    // BOM in UTF-32(BE|LE)
    using std::string_literals::operator""s;
    const std::string kUtf32BeBom = "\x00\x00\xFE\xFF"s;
    EXPECT_FALSE(fn(kUtf32BeBom));
    const std::string kUtf32LeBom = "\xFF\xFE\x00\x00"s;
    EXPECT_FALSE(fn(kUtf32LeBom));
}

}  // namespace

TEST(StringUtilTest, IsStringUTF8) {
    TestStructurallyValidUtf8(&is_string_utf8);
    TestStructurallyInvalidUtf8(&is_string_utf8);
}

TEST(StringUtilTest, IsStringASCII) {
    static char char_ascii[] = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    static char16_t char16_ascii[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A',
                                      'B', 'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6',
                                      '7', '8', '9', '0', 'A', 'B', 'C', 'D', 'E', 'F', 0};
    static std::wstring wchar_ascii(L"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");

    // Test a variety of the fragment start positions and lengths in order to make
    // sure that bit masking in IsStringASCII works correctly.
    // Also, test that a non-ASCII character will be detected regardless of its
    // position inside the string.
    {
        const size_t string_length = std::size(char_ascii) - 1;
        for (size_t offset = 0; offset < 8; ++offset) {
            for (size_t len = 0, max_len = string_length - offset; len < max_len; ++len) {
                EXPECT_TRUE(is_string_ascii(std::string_view(char_ascii + offset, len)));
                for (size_t char_pos = offset; char_pos < len; ++char_pos) {
                    char_ascii[char_pos] |= '\x80';
                    EXPECT_FALSE(is_string_ascii(std::string_view(char_ascii + offset, len)));
                    char_ascii[char_pos] &= ~'\x80';
                }
            }
        }
    }

    {
        const size_t string_length = std::size(char16_ascii) - 1;
        for (size_t offset = 0; offset < 4; ++offset) {
            for (size_t len = 0, max_len = string_length - offset; len < max_len; ++len) {
                EXPECT_TRUE(is_string_ascii(std::u16string_view(char16_ascii + offset, len)));
                for (size_t char_pos = offset; char_pos < len; ++char_pos) {
                    char16_ascii[char_pos] |= 0x80;
                    EXPECT_FALSE(is_string_ascii(std::u16string_view(char16_ascii + offset, len)));
                    char16_ascii[char_pos] &= ~0x80;
                    // Also test when the upper half is non-zero.
                    char16_ascii[char_pos] |= 0x100;
                    EXPECT_FALSE(is_string_ascii(std::u16string_view(char16_ascii + offset, len)));
                    char16_ascii[char_pos] &= ~0x100;
                }
            }
        }
    }
}

}  // namespace base
