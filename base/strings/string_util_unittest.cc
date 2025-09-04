#include "base/strings/string_util.h"
#include <gtest/gtest.h>

namespace base {

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
                EXPECT_TRUE(IsStringASCII(std::string_view(char_ascii + offset, len)));
                for (size_t char_pos = offset; char_pos < len; ++char_pos) {
                    char_ascii[char_pos] |= '\x80';
                    EXPECT_FALSE(IsStringASCII(std::string_view(char_ascii + offset, len)));
                    char_ascii[char_pos] &= ~'\x80';
                }
            }
        }
    }

    {
        const size_t string_length = std::size(char16_ascii) - 1;
        for (size_t offset = 0; offset < 4; ++offset) {
            for (size_t len = 0, max_len = string_length - offset; len < max_len; ++len) {
                EXPECT_TRUE(IsStringASCII(std::u16string_view(char16_ascii + offset, len)));
                for (size_t char_pos = offset; char_pos < len; ++char_pos) {
                    char16_ascii[char_pos] |= 0x80;
                    EXPECT_FALSE(IsStringASCII(std::u16string_view(char16_ascii + offset, len)));
                    char16_ascii[char_pos] &= ~0x80;
                    // Also test when the upper half is non-zero.
                    char16_ascii[char_pos] |= 0x100;
                    EXPECT_FALSE(IsStringASCII(std::u16string_view(char16_ascii + offset, len)));
                    char16_ascii[char_pos] &= ~0x100;
                }
            }
        }
    }
}

}  // namespace base
