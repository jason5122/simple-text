#include "experiments/unicode/unicode.h"

#define USE_SIMDUTF

#ifdef USE_SIMDUTF
#include "third_party/simdutf/simdutf.h"
#endif

namespace unicode {

#ifdef USE_SIMDUTF

std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string utf16;
    utf16.resize_and_overwrite(
        simdutf::utf16_length_from_utf8(utf8.data(), utf8.size()), [&](char16_t* out, size_t) {
            return simdutf::convert_valid_utf8_to_utf16(utf8.data(), utf8.size(), out);
        });
    return utf16;
}

std::u32string utf8_to_utf32(std::string_view utf8) {
    std::u32string utf32;
    utf32.resize_and_overwrite(
        simdutf::utf32_length_from_utf8(utf8.data(), utf8.size()), [&](char32_t* out, size_t) {
            return simdutf::convert_valid_utf8_to_utf32(utf8.data(), utf8.size(), out);
        });
    return utf32;
}

std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string utf8;
    utf8.resize_and_overwrite(
        simdutf::utf8_length_from_utf16(utf16.data(), utf16.size()), [&](char* out, size_t) {
            return simdutf::convert_valid_utf16_to_utf8(utf16.data(), utf16.size(), out);
        });
    return utf8;
}

std::u32string utf16_to_utf32(std::u16string_view utf16) {
    std::u32string utf32;
    utf32.resize_and_overwrite(
        simdutf::utf32_length_from_utf16(utf16.data(), utf16.size()), [&](char32_t* out, size_t) {
            return simdutf::convert_valid_utf16_to_utf32(utf16.data(), utf16.size(), out);
        });
    return utf32;
}

std::string utf32_to_utf8(std::u32string_view utf32) {
    std::string utf8;
    utf8.resize_and_overwrite(
        simdutf::utf8_length_from_utf32(utf32.data(), utf32.size()), [&](char* out, size_t) {
            return simdutf::convert_valid_utf32_to_utf8(utf32.data(), utf32.size(), out);
        });
    return utf8;
}

std::u16string utf32_to_utf16(std::u32string_view utf32) {
    std::u16string utf16;
    utf16.resize_and_overwrite(
        simdutf::utf16_length_from_utf32(utf32.data(), utf32.size()), [&](char16_t* out, size_t) {
            return simdutf::convert_valid_utf32_to_utf16(utf32.data(), utf32.size(), out);
        });
    return utf16;
}

#else

std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string utf16;
    // No UTF-8 sequence needs more UTF-16 code units than it has bytes.
    utf16.resize_and_overwrite(utf8.size(), [&](char16_t* out, size_t) {
        size_t i = 0, j = 0;
        while (i < utf8.size()) {
            encode_utf16(out, j, decode_utf8(utf8, i));
        }
        return j;
    });
    return utf16;
}

std::u32string utf8_to_utf32(std::string_view utf8) {
    std::u32string utf32;
    utf32.resize_and_overwrite(utf8.size(), [&](char32_t* out, size_t) {
        size_t i = 0, j = 0;
        while (i < utf8.size()) {
            out[j++] = decode_utf8(utf8, i);
        }
        return j;
    });
    return utf32;
}

std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string utf8;
    // A BMP code unit needs at most 3 bytes; a surrogate pair needs 4 for its 2.
    utf8.resize_and_overwrite(utf16.size() * 3, [&](char* out, size_t) {
        size_t i = 0, j = 0;
        while (i < utf16.size()) {
            encode_utf8(out, j, decode_utf16(utf16, i));
        }
        return j;
    });
    return utf8;
}

std::u32string utf16_to_utf32(std::u16string_view utf16) {
    std::u32string utf32;
    utf32.resize_and_overwrite(utf16.size(), [&](char32_t* out, size_t) {
        size_t i = 0, j = 0;
        while (i < utf16.size()) {
            out[j++] = decode_utf16(utf16, i);
        }
        return j;
    });
    return utf32;
}

std::string utf32_to_utf8(std::u32string_view utf32) {
    std::string utf8;
    utf8.resize_and_overwrite(utf32.size() * 4, [&](char* out, size_t) {
        size_t j = 0;
        for (char32_t cp : utf32) {
            encode_utf8(out, j, cp);
        }
        return j;
    });
    return utf8;
}

std::u16string utf32_to_utf16(std::u32string_view utf32) {
    std::u16string utf16;
    utf16.resize_and_overwrite(utf32.size() * 2, [&](char16_t* out, size_t) {
        size_t j = 0;
        for (char32_t cp : utf32) {
            encode_utf16(out, j, cp);
        }
        return j;
    });
    return utf16;
}

#endif  // USE_SIMDUTF

}  // namespace unicode
