#include "base/check.h"
#include "base/unicode.h"
#include "third_party/simdutf/simdutf.h"

namespace base {

bool is_valid_utf8(std::string_view utf8) { return simdutf::validate_utf8(utf8); }
bool is_valid_utf16(std::u16string_view utf16) { return simdutf::validate_utf16(utf16); }
bool is_valid_utf32(std::u32string_view utf32) { return simdutf::validate_utf32(utf32); }

std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string utf16;
    utf16.resize_and_overwrite(
        simdutf::utf16_length_from_utf8(utf8), [&](char16_t* out, size_t n) {
            return simdutf::convert_valid_utf8_to_utf16(utf8, std::span(out, n));
        });
    return utf16;
}

std::u32string utf8_to_utf32(std::string_view utf8) {
    std::u32string utf32;
    utf32.resize_and_overwrite(
        simdutf::utf32_length_from_utf8(utf8), [&](char32_t* out, size_t n) {
            return simdutf::convert_valid_utf8_to_utf32(utf8, std::span(out, n));
        });
    return utf32;
}

std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string utf8;
    utf8.resize_and_overwrite(simdutf::utf8_length_from_utf16(utf16), [&](char* out, size_t n) {
        return simdutf::convert_valid_utf16_to_utf8(utf16, std::span(out, n));
    });
    return utf8;
}

std::u32string utf16_to_utf32(std::u16string_view utf16) {
    std::u32string utf32;
    utf32.resize_and_overwrite(
        simdutf::utf32_length_from_utf16(utf16), [&](char32_t* out, size_t n) {
            return simdutf::convert_valid_utf16_to_utf32(utf16, std::span(out, n));
        });
    return utf32;
}

std::string utf32_to_utf8(std::u32string_view utf32) {
    std::string utf8;
    utf8.resize_and_overwrite(simdutf::utf8_length_from_utf32(utf32), [&](char* out, size_t n) {
        return simdutf::convert_valid_utf32_to_utf8(utf32, std::span(out, n));
    });
    return utf8;
}

std::u16string utf32_to_utf16(std::u32string_view utf32) {
    std::u16string utf16;
    utf16.resize_and_overwrite(
        simdutf::utf16_length_from_utf32(utf32), [&](char16_t* out, size_t n) {
            return simdutf::convert_valid_utf32_to_utf16(utf32, std::span(out, n));
        });
    return utf16;
}

std::vector<size_t> utf16_to_utf8_offsets(std::string_view utf8) {
    DCHECK(is_valid_utf8(utf8));

    std::vector<size_t> offsets(simdutf::utf16_length_from_utf8(utf8));
    size_t j = 0;
    for (size_t i = 0; i < utf8.size();) {
        size_t start = i;
        char32_t cp = decode_valid_utf8(utf8, i);
        offsets[j++] = start;
        // Supplementary code points take a surrogate pair.
        if (cp > 0xffff) offsets[j++] = start;
    }
    return offsets;
}

}  // namespace base
