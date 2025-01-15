#pragma once

#include <cstddef>
#include <cstdint>

namespace unicode {

using Unichar = int32_t;

/** Given a sequence of UTF-8 bytes, return the number of unicode codepoints.
    If the sequence is invalid UTF-8, return -1.
*/
int CountUTF8(const char* utf8, size_t byteLength);

/** Given a sequence of aligned UTF-16 characters in machine-endian form,
    return the number of unicode codepoints.  If the sequence is invalid
    UTF-16, return -1.
*/
int CountUTF16(const uint16_t* utf16, size_t byteLength);

/** Given a sequence of aligned UTF-32 characters in machine-endian form,
    return the number of unicode codepoints.  If the sequence is invalid
    UTF-32, return -1.
*/
int CountUTF32(const int32_t* utf32, size_t byteLength);

/** Given a sequence of UTF-8 bytes, return the first unicode codepoint.
    The pointer will be incremented to point at the next codepoint's start.  If
    invalid UTF-8 is encountered, set *ptr to end and return -1.
*/
Unichar NextUTF8(const char** ptr, const char* end);

/** Given a sequence of aligned UTF-16 characters in machine-endian form,
    return the first unicode codepoint.  The pointer will be incremented to
    point at the next codepoint's start.  If invalid UTF-16 is encountered,
    set *ptr to end and return -1.
*/
Unichar NextUTF16(const uint16_t** ptr, const uint16_t* end);

/** Given a sequence of aligned UTF-32 characters in machine-endian form,
    return the first unicode codepoint.  The pointer will be incremented to
    point at the next codepoint's start.  If invalid UTF-32 is encountered,
    set *ptr to end and return -1.
*/
Unichar NextUTF32(const int32_t** ptr, const int32_t* end);

constexpr unsigned kMaxBytesInUTF8Sequence = 4;

/** Convert the unicode codepoint into UTF-8.  If `utf8` is non-null, place the
    result in that array.  Return the number of bytes in the result.  If `utf8`
    is null, simply return the number of bytes that would be used.  For invalid
    unicode codepoints, return 0.
*/
size_t ToUTF8(Unichar uni, char utf8[kMaxBytesInUTF8Sequence] = nullptr);

/** Convert the unicode codepoint into UTF-16.  If `utf16` is non-null, place
    the result in that array.  Return the number of UTF-16 code units in the
    result (1 or 2).  If `utf16` is null, simply return the number of code
    units that would be used.  For invalid unicode codepoints, return 0.
*/
size_t ToUTF16(Unichar uni, uint16_t utf16[2] = nullptr);

/** Returns the number of resulting UTF16 values needed to convert the src utf8 sequence.
 *  If dst is not null, it is filled with the corresponding values up to its capacity.
 *  If there is an error, -1 is returned and the dst[] buffer is undefined.
 */
int UTF8ToUTF16(uint16_t dst[], int dstCapacity, const char src[], size_t srcByteLength);

/** Returns the number of resulting UTF8 values needed to convert the src utf16 sequence.
 *  If dst is not null, it is filled with the corresponding values up to its capacity.
 *  If there is an error, -1 is returned and the dst[] buffer is undefined.
 */
int UTF16ToUTF8(char dst[], int dstCapacity, const uint16_t src[], size_t srcLength);

/**
 * Given a UTF-16 code point, returns true iff it is a leading surrogate.
 * https://unicode.org/faq/utf_bom.html#utf16-2
 */
constexpr bool IsLeadingSurrogateUTF16(uint16_t c) {
    return ((c) & 0xFC00) == 0xD800;
}

/**
 * Given a UTF-16 code point, returns true iff it is a trailing surrogate.
 * https://unicode.org/faq/utf_bom.html#utf16-2
 */
constexpr bool IsTrailingSurrogateUTF16(uint16_t c) {
    return ((c) & 0xFC00) == 0xDC00;
}

}  // namespace unicode
