#include "base/third_party/icu/icu_utf.h"
#include "base/unicode/unicode.h"

namespace base {

int count_utf8(std::string_view utf8) {
    const auto* src = reinterpret_cast<const uint8_t*>(utf8.data());
    size_t len = utf8.length();

    int count = 0;
    for (size_t i = 0; i < len;) {
        base_icu::UChar32 cp;
        CBU8_NEXT(src, i, len, cp);
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
        CBU16_NEXT(src, i, len, cp);
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
        CBU8_NEXT(src, i, len, cp);
        if (is_valid_codepoint(cp)) return cp;
    }
    return -1;
}

Unichar next_utf16(std::u16string_view utf16, size_t& i) {
    const auto* src = reinterpret_cast<const uint16_t*>(utf16.data());
    size_t len = utf16.length();

    if (i < len) {
        base_icu::UChar32 cp;
        CBU16_NEXT(src, i, len, cp);
        if (is_valid_codepoint(cp)) return cp;
    }
    return -1;
}

int codepoint_to_utf8(Unichar cp, char utf8[base::kMaxBytesInUTF8Sequence]) {
    if (!is_valid_codepoint(cp)) return -1;

    if (utf8) {
        size_t _ = 0;
        CBU8_APPEND_UNSAFE(reinterpret_cast<uint8_t*>(utf8), _, cp);
    }
    return CBU8_LENGTH(cp);
}

int codepoint_to_utf16(Unichar cp, uint16_t utf16[2]) {
    if (!is_valid_codepoint(cp)) return -1;

    if (utf16) {
        size_t _ = 0;
        CBU16_APPEND_UNSAFE(utf16, _, cp);
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
