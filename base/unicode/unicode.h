#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace base {

using Unichar = int32_t;
constexpr unsigned kMaxBytesInUTF8Sequence = 4;

// Given a UTF-8 string, return the number of unicode codepoints.
// If the sequence is invalid UTF-8, return -1.
int count_utf8(std::string_view utf8);

// Given a UTF-16 string, return the number of unicode codepoints.
// If the sequence is invalid UTF-16, return -1.
int count_utf16(std::u16string_view utf16);

// Given a UTF-8 string, return the first unicode codepoint. The index will be incremented to point
// at the next codepoint's start.
// If invalid UTF-8 is encountered, return -1.
Unichar next_utf8(std::string_view utf8, size_t& i);

// Given a UTF-16 string, return the first unicode codepoint. The index will be incremented to
// point at the next codepoint's start.
// If invalid UTF-8 is encountered, return -1.
Unichar next_utf16(std::u16string_view utf16, size_t& i);

// Convert the unicode codepoint into UTF-8.  If `utf8` is non-null, place the result in that
// array.  Return the number of bytes in the result.  If `utf8` is null, simply return the number
// of bytes that would be used.  For invalid unicode codepoints, return -1.
int codepoint_to_utf8(Unichar cp, char utf8[kMaxBytesInUTF8Sequence] = nullptr);

// Convert the unicode codepoint into UTF-16.  If `utf16` is non-null, place the result in that
// array.  Return the number of UTF-16 code units in the result (1 or 2).  If `utf16` is null,
// simply return the number of code units that would be used.  For invalid unicode codepoints,
// return -1.
int codepoint_to_utf16(Unichar cp, uint16_t utf16[2] = nullptr);

constexpr bool is_valid_codepoint(Unichar cp) {
    // Excludes code points that are not Unicode scalar values, i.e.
    // surrogate code points ([0xD800, 0xDFFF]). Additionally, excludes
    // code points larger than 0x10FFFF (the highest codepoint allowed).
    // Non-characters and unassigned code points are allowed.
    // https://unicode.org/glossary/#unicode_scalar_value
    return (cp >= 0 && cp < 0xD800) || (cp >= 0xE000 && cp <= 0x10FFFF);
}

}  // namespace base
