#pragma once

#include "third_party/icu/icu_utf.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace base {

bool is_valid_utf8(std::string_view utf8);
bool is_valid_utf16(std::u16string_view utf16);
bool is_valid_utf32(std::u32string_view utf32);

std::u16string utf8_to_utf16(std::string_view utf8);
std::u32string utf8_to_utf32(std::string_view utf8);
std::string utf16_to_utf8(std::u16string_view utf16);
std::u32string utf16_to_utf32(std::u16string_view utf16);
std::string utf32_to_utf8(std::u32string_view utf32);
std::u16string utf32_to_utf16(std::u32string_view utf32);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wsign-conversion"

inline char32_t decode_utf8(std::string_view utf8, size_t& i) {
    char32_t cp;
    U8_NEXT_OR_FFFD(utf8, i, utf8.size(), cp);
    return cp;
}

inline char32_t decode_utf16(std::u16string_view utf16, size_t& i) {
    char32_t cp;
    U16_NEXT_OR_FFFD(utf16, i, utf16.size(), cp);
    return cp;
}

inline char32_t decode_valid_utf8(std::string_view utf8, size_t& i) {
    char32_t cp;
    U8_NEXT_UNSAFE(utf8, i, cp);
    return cp;
}

inline char32_t decode_valid_utf16(std::u16string_view utf16, size_t& i) {
    char32_t cp;
    U16_NEXT_UNSAFE(utf16, i, cp);
    return cp;
}

#pragma clang diagnostic pop

// TODO: This is just to replace our old DFA-based reverse decoder. Replace with ICU's reverse
// algorithms once we migrate to rope with a chunk-based walker.
/*
 * Number of bytes in the UTF-8 sequence that `lead` begins: 2..4 for a lead byte, otherwise 1.
 * `decode_utf8` consumes exactly this many bytes from a well-formed sequence and never more.
 */
inline size_t utf8_sequence_length(char lead) {
    return 1 + static_cast<size_t>(U8_COUNT_TRAIL_BYTES(lead));
}

// TODO: This is just to replace our old DFA-based reverse decoder. Replace with ICU's reverse
// algorithms once we migrate to rope with a chunk-based walker.
inline char32_t decode_prev_utf8(std::string_view utf8, size_t& i) {
    // Back up over trail bytes to the candidate lead byte (at most three of them), then decode
    // forward from it. The candidate is right only if that decode lands exactly on `i`.
    const size_t end = i;
    const size_t max_len = std::min(end, 4UZ);
    for (size_t len = 1; len <= max_len; ++len) {
        if (U8_IS_TRAIL(utf8[end - len])) continue;
        size_t j = end - len;
        char32_t cp = decode_utf8(utf8.substr(0, end), j);
        if (j != end) break;
        i = end - len;
        return cp;
    }
    // A stray trail byte, or a lead byte whose sequence ends before `i`.
    i = end - 1;
    return 0xFFFD;
}

/*
 * Maps each UTF-16 code unit of `utf8` (as it would be re-encoded in UTF-16) to the byte offset of
 * its code point in `utf8`. Both halves of a surrogate pair map to the same offset.
 *
 * Precondition: input is valid UTF-8.
 */
std::vector<size_t> utf16_to_utf8_offsets(std::string_view utf8);

}  // namespace base
