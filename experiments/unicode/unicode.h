#pragma once

#include "experiments/unicode/third_party/icu/icu_utf.h"
#include <cstddef>
#include <string>
#include <string_view>

// TODO: Add error handling.

namespace unicode {

inline char32_t decode_utf8(std::string_view utf8, size_t& i) {
    char32_t cp;
    U8_NEXT_UNSAFE(utf8, i, cp);
    return cp;
}

inline char32_t decode_utf16(std::u16string_view utf16, size_t& i) {
    char32_t cp;
    U16_NEXT_UNSAFE(utf16, i, cp);
    return cp;
}

inline void encode_utf8(char* utf8, size_t& i, char32_t cp) { U8_APPEND_UNSAFE(utf8, i, cp); }

inline void encode_utf16(char16_t* utf16, size_t& i, char32_t cp) {
    U16_APPEND_UNSAFE(utf16, i, cp);
}

std::u16string utf8_to_utf16(std::string_view utf8);
std::u32string utf8_to_utf32(std::string_view utf8);
std::string utf16_to_utf8(std::u16string_view utf16);
std::u32string utf16_to_utf32(std::u16string_view utf16);
std::string utf32_to_utf8(std::u32string_view utf32);
std::u16string utf32_to_utf16(std::u32string_view utf32);

}  // namespace unicode
