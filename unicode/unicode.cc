#include "unicode/unicode.h"
#include <utility>

namespace unicode {

namespace {

constexpr int32_t left_shift(int32_t value, int32_t shift) {
    return (int32_t)((uint32_t)value << shift);
}

/** @returns   -1  iff invalid UTF8 byte,
                0  iff UTF8 continuation byte,
                1  iff ASCII byte,
                2  iff leading byte of 2-byte sequence,
                3  iff leading byte of 3-byte sequence, and
                4  iff leading byte of 4-byte sequence.
      I.e.: if return value > 0, then gives length of sequence.
*/
constexpr int utf8_byte_type(uint8_t c) {
    if (c < 0x80) return 1;            // ASCII
    if ((c & 0xC0) == 0x80) return 0;  // continuation 10xxxxxx

    // Disallow illegal lead bytes up-front.
    if (c < 0xC2) return -1;  // C0/C1 are invalid (overlong 2-byte)
    if (c > 0xF4) return -1;  // F5..FF invalid (outside Unicode range)

    if ((c & 0xE0) == 0xC0) return 2;  // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;  // 11110xxx

    return -1;
}
constexpr bool utf8_type_is_valid_leading_byte(int type) { return type > 0; }
constexpr bool utf8_byte_is_continuation(uint8_t c) { return utf8_byte_type(c) == 0; }

// https://unicode.org/faq/utf_bom.html#utf16-2
constexpr bool utf16_is_high_surrogate(uint16_t c) { return (c & 0xFC00) == 0xD800; }
constexpr bool utf16_is_low_surrogate(uint16_t c) { return (c & 0xFC00) == 0xDC00; }

template <typename T>
constexpr Unichar next_fail(const T** ptr, const T* end) {
    *ptr = end;
    return -1;
}

}  // namespace

int count_utf8(std::string_view str8) {
    int count = 0;
    size_t i = 0;
    size_t n = str8.length();
    while (i < n) {
        int len = utf8_byte_type(str8[i]);
        if (len <= 0 || i + len > n) return -1;

        for (size_t k = 1; k < len; ++k) {
            if (!utf8_byte_is_continuation(str8[i + k])) return -1;
        }
        i += len;
        ++count;
    }
    return count;
}

int count_utf16(std::u16string_view str16) {
    size_t i = 0;
    int count = 0;
    while (i < str16.length()) {
        char16_t c = str16[i++];
        if (utf16_is_low_surrogate(c)) return -1;
        if (utf16_is_high_surrogate(c)) {
            if (i >= str16.length()) return -1;
            c = str16[i++];
            if (!utf16_is_low_surrogate(c)) return -1;
        }
        ++count;
    }
    return count;
}

Unichar next_utf8(const char** ptr, const char* end) {
    if (!ptr || !end) {
        return -1;
    }
    const uint8_t* p = (const uint8_t*)*ptr;
    if (!p || p >= (const uint8_t*)end) {
        return next_fail(ptr, end);
    }
    int c = *p;
    int hic = c << 24;

    if (!utf8_type_is_valid_leading_byte(utf8_byte_type(c))) {
        return next_fail(ptr, end);
    }
    if (hic < 0) {
        uint32_t mask = (uint32_t)~0x3F;
        hic = left_shift(hic, 1);
        do {
            ++p;
            if (p >= (const uint8_t*)end) {
                return next_fail(ptr, end);
            }
            // check before reading off end of array.
            uint8_t nextByte = *p;
            if (!utf8_byte_is_continuation(nextByte)) {
                return next_fail(ptr, end);
            }
            c = (c << 6) | (nextByte & 0x3F);
            mask <<= 5;
        } while ((hic = left_shift(hic, 1)) < 0);
        c &= ~mask;
    }
    *ptr = (char*)p + 1;
    return c;
}

Unichar next_utf16(const uint16_t** ptr, const uint16_t* end) {
    if (!ptr || !end) {
        return -1;
    }
    const uint16_t* src = *ptr;
    if (!src || src + 1 > end) {
        return next_fail(ptr, end);
    }
    uint16_t c = *src++;
    Unichar result = c;
    if (utf16_is_low_surrogate(c)) {
        return next_fail(ptr, end);  // srcPtr should never point at low surrogate.
    }
    if (utf16_is_high_surrogate(c)) {
        if (src + 1 > end) {
            return next_fail(ptr, end);  // Truncated string.
        }
        uint16_t low = *src++;
        if (!utf16_is_low_surrogate(low)) {
            return next_fail(ptr, end);
        }
        /*
        [paraphrased from wikipedia]
        Take the high surrogate and subtract 0xD800, then multiply by 0x400.
        Take the low surrogate and subtract 0xDC00.  Add these two results
        together, and finally add 0x10000 to get the final decoded codepoint.

        unicode = (high - 0xD800) * 0x400 + low - 0xDC00 + 0x10000
        unicode = (high * 0x400) - (0xD800 * 0x400) + low - 0xDC00 + 0x10000
        unicode = (high << 10) - (0xD800 << 10) + low - 0xDC00 + 0x10000
        unicode = (high << 10) + low - ((0xD800 << 10) + 0xDC00 - 0x10000)
        */
        result = (result << 10) + (Unichar)low - ((0xD800 << 10) + 0xDC00 - 0x10000);
    }
    *ptr = src;
    return result;
}

size_t to_utf8(Unichar uni, char utf8[unicode::kMaxBytesInUTF8Sequence]) {
    if ((uint32_t)uni > 0x10FFFF) {
        return 0;
    }
    if (uni <= 127) {
        if (utf8) {
            *utf8 = (char)uni;
        }
        return 1;
    }
    char tmp[4];
    char* p = tmp;
    size_t count = 1;
    while (uni > 0x7F >> count) {
        *p++ = (char)(0x80 | (uni & 0x3F));
        uni >>= 6;
        count += 1;
    }
    if (utf8) {
        p = tmp;
        utf8 += count;
        while (p < tmp + count - 1) {
            *--utf8 = *p++;
        }
        *--utf8 = (char)(~(0xFF >> count) | uni);
    }
    return count;
}

size_t to_utf16(Unichar uni, uint16_t utf16[2]) {
    if ((uint32_t)uni > 0x10FFFF) {
        return 0;
    }
    int extra = (uni > 0xFFFF);
    if (utf16) {
        if (extra) {
            utf16[0] = (uint16_t)((0xD800 - 64) + (uni >> 10));
            utf16[1] = (uint16_t)(0xDC00 | (uni & 0x3FF));
        } else {
            utf16[0] = (uint16_t)uni;
        }
    }
    return 1 + extra;
}

int utf8_to_utf16(uint16_t dst[], int dstCapacity, const char src[], size_t srcByteLength) {
    if (!dst) {
        dstCapacity = 0;
    }

    int dstLength = 0;
    uint16_t* endDst = dst + dstCapacity;
    const char* endSrc = src + srcByteLength;
    while (src < endSrc) {
        Unichar uni = next_utf8(&src, endSrc);
        if (uni < 0) {
            return -1;
        }

        uint16_t utf16[2];
        size_t count = to_utf16(uni, utf16);
        if (count == 0) {
            return -1;
        }
        dstLength += count;

        if (dst) {
            uint16_t* elems = utf16;
            while (dst < endDst && count > 0) {
                *dst++ = *elems++;
                count -= 1;
            }
        }
    }
    return dstLength;
}

int utf16_to_utf8(char dst[], int dstCapacity, const uint16_t src[], size_t srcLength) {
    if (!dst) {
        dstCapacity = 0;
    }

    int dstLength = 0;
    const char* endDst = dst + dstCapacity;
    const uint16_t* endSrc = src + srcLength;
    while (src < endSrc) {
        Unichar uni = next_utf16(&src, endSrc);
        if (uni < 0) {
            return -1;
        }

        char utf8[unicode::kMaxBytesInUTF8Sequence];
        size_t count = to_utf8(uni, utf8);
        if (count == 0) {
            return -1;
        }
        dstLength += count;

        if (dst) {
            const char* elems = utf8;
            while (dst < endDst && count > 0) {
                *dst++ = *elems++;
                count -= 1;
            }
        }
    }
    return dstLength;
}

}  // namespace unicode
