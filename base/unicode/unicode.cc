#include "base/compiler_specific.h"
#include "base/third_party/icu/icu_utf.h"
#include "base/unicode/unicode.h"

namespace base {

std::string utf32_to_utf8(std::u32string_view input) {
    std::string output;
    output.reserve(input.size());
    for (const uint32_t cp : input) {
        if (cp <= 0x7f) {
            output.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0x10ffff) {
            output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return output;
}

int count_utf8(std::string_view utf8) {
    const auto* src = reinterpret_cast<const uint8_t*>(utf8.data());
    size_t len = utf8.length();

    int count = 0;
    for (size_t i = 0; i < len;) {
        base_icu::UChar32 cp;
        UNSAFE_TODO(CBU8_NEXT(src, i, len, cp));
        if (!is_valid_codepoint(cp)) return -1;
        ++count;
    }
    return count;
}

int count_utf16(std::u16string_view utf16) {
    const auto* src = reinterpret_cast<const uint16_t*>(utf16.data());
    size_t len = utf16.length();

    int count = 0;
    for (size_t i = 0; i < len;) {
        base_icu::UChar32 cp;
        UNSAFE_TODO(CBU16_NEXT(src, i, len, cp));
        if (!is_valid_codepoint(cp)) return -1;
        ++count;
    }
    return count;
}

Unichar next_utf8(std::string_view utf8, size_t& i) {
    const auto* src = reinterpret_cast<const uint8_t*>(utf8.data());
    size_t len = utf8.length();

    if (i < len) {
        base_icu::UChar32 cp;
        UNSAFE_TODO(CBU8_NEXT(src, i, len, cp));
        if (is_valid_codepoint(cp)) return cp;
    }
    return -1;
}

Unichar next_utf16(std::u16string_view utf16, size_t& i) {
    const auto* src = reinterpret_cast<const uint16_t*>(utf16.data());
    size_t len = utf16.length();

    if (i < len) {
        base_icu::UChar32 cp;
        UNSAFE_TODO(CBU16_NEXT(src, i, len, cp));
        if (is_valid_codepoint(cp)) return cp;
    }
    return -1;
}

int codepoint_to_utf8(Unichar cp, char utf8[base::kMaxBytesInUTF8Sequence]) {
    if (!is_valid_codepoint(cp)) return -1;

    if (utf8) {
        size_t _ = 0;
        UNSAFE_TODO(CBU8_APPEND_UNSAFE(reinterpret_cast<uint8_t*>(utf8), _, cp));
    }
    return CBU8_LENGTH(cp);
}

int codepoint_to_utf16(Unichar cp, uint16_t utf16[2]) {
    if (!is_valid_codepoint(cp)) return -1;

    if (utf16) {
        size_t _ = 0;
        UNSAFE_TODO(CBU16_APPEND_UNSAFE(utf16, _, cp));
    }
    return CBU16_LENGTH(cp);
}

int utf8_to_utf16_length(std::string_view utf8) {
    int len = 0;
    for (size_t i = 0; i < utf8.length();) {
        Unichar cp = next_utf8(utf8, i);
        if (cp < 0) return -1;

        int count = codepoint_to_utf16(cp);
        if (count < 0) return -1;

        len += count;
    }
    return len;
}

}  // namespace base
