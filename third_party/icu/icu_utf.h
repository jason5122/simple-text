#pragma once

#include <cstdint>

namespace icu_utf {

// clang-format off

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// source/common/unicode/umachine.h
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * \var UChar
 *
 * The base type for UTF-16 code units and pointers.
 * Unsigned 16-bit integer.
 * Starting with ICU 59, C++ API uses char16_t directly, while C API continues to use UChar.
 *
 * UChar is configurable by defining the macro UCHAR_TYPE
 * on the preprocessor or compiler command line:
 * -DUCHAR_TYPE=uint16_t or -DUCHAR_TYPE=wchar_t (if U_SIZEOF_WCHAR_T==2) etc.
 * (The UCHAR_TYPE can also be \#defined earlier in this file, for outside the ICU library code.)
 * This is for transitional use from application code that uses uint16_t or wchar_t for UTF-16.
 *
 * The default is UChar=char16_t.
 *
 * C++11 defines char16_t as bit-compatible with uint16_t, but as a distinct type.
 *
 * In C, char16_t is a simple typedef of uint_least16_t.
 * ICU requires uint_least16_t=uint16_t for data memory mapping.
 * On macOS, char16_t is not available because the uchar.h standard header is missing.
 *
 * @stable ICU 4.4
 */

#if 1
    // #if 1 is normal. UChar defaults to char16_t in C++.
    // For configuration testing of UChar=uint16_t temporarily change this to #if 0.
#else
#   define UCHAR_TYPE uint16_t
#endif

#if defined(U_ALL_IMPLEMENTATION) || !defined(UCHAR_TYPE)
    typedef char16_t UChar;
#else
    typedef UCHAR_TYPE UChar;
#endif

/**
 * Define UChar32 as a type for single Unicode code points.
 * UChar32 is a signed 32-bit integer (same as int32_t).
 *
 * The Unicode code point range is 0..0x10ffff.
 * All other values (negative or >=0x110000) are illegal as Unicode code points.
 * They may be used as sentinel values to indicate "done", "error"
 * or similar non-code point conditions.
 *
 * Before ICU 2.4 (Jitterbug 2146), UChar32 was defined
 * to be wchar_t if that is 32 bits wide (wchar_t may be signed or unsigned)
 * or else to be uint32_t.
 * That is, the definition of UChar32 was platform-dependent.
 *
 * @see U_SENTINEL
 * @stable ICU 2.4
 */
typedef int32_t UChar32;

/**
 * \def UPRV_BLOCK_MACRO_BEGIN
 * Defined as the "do" keyword by default.
 * @internal
 */
#ifndef UPRV_BLOCK_MACRO_BEGIN
#define UPRV_BLOCK_MACRO_BEGIN do
#endif

/**
 * \def UPRV_BLOCK_MACRO_END
 * Defined as "while (false)" by default.
 * @internal
 */
#ifndef UPRV_BLOCK_MACRO_END
#define UPRV_BLOCK_MACRO_END while (false)
#endif

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// source/common/unicode/utf.h
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * Is this code point a surrogate (U+d800..U+dfff)?
 * @param c 32-bit code point
 * @return true or false
 * @stable ICU 2.4
 */
#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// source/common/unicode/utf8.h
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * Counts the trail bytes for a UTF-8 lead byte.
 * Returns 0 for 0..0xc1 as well as for 0xf5..0xff.
 * leadByte might be evaluated multiple times.
 *
 * This is internal since it is not meant to be called directly by external clients;
 * however it is called by public macros in this file and thus must remain stable.
 *
 * @param leadByte The first byte of a UTF-8 sequence. Must be 0..0xff.
 * @internal
 */
#define U8_COUNT_TRAIL_BYTES(leadByte) \
    (U8_IS_LEAD(leadByte) ? \
        ((uint8_t)(leadByte)>=0xe0)+((uint8_t)(leadByte)>=0xf0)+1 : 0)

/**
 * Internal bit vector for 3-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD3_AND_T1.
 * Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
 * Lead byte E0..EF bits 3..0 are used as byte index,
 * first trail byte bits 7..5 are used as bit index into that byte.
 * @see U8_IS_VALID_LEAD3_AND_T1
 * @internal
 */
#define U8_LEAD3_T1_BITS "\x20\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x10\x30\x30"

/**
 * Internal bit vector for 4-byte UTF-8 validity check, for use in U8_IS_VALID_LEAD4_AND_T1.
 * Each bit indicates whether one lead byte + first trail byte pair starts a valid sequence.
 * First trail byte bits 7..4 are used as byte index,
 * lead byte F0..F4 bits 2..0 are used as bit index into that byte.
 * @see U8_IS_VALID_LEAD4_AND_T1
 * @internal
 */
#define U8_LEAD4_T1_BITS "\x00\x00\x00\x00\x00\x00\x00\x00\x1E\x0F\x0F\x0F\x00\x00\x00\x00"

/**
 * Does this code unit (byte) encode a code point by itself (US-ASCII 0..0x7f)?
 * @param c 8-bit code unit (byte)
 * @return true or false
 * @stable ICU 2.4
 */
#define U8_IS_SINGLE(c) ((int8_t)(c)>=0)

/**
 * Is this code unit (byte) a UTF-8 lead byte? (0xC2..0xF4)
 * @param c 8-bit code unit (byte)
 * @return true or false
 * @stable ICU 2.4
 */
#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc2)<=0x32)

/**
 * Is this code unit (byte) a UTF-8 trail byte? (0x80..0xBF)
 * @param c 8-bit code unit (byte)
 * @return true or false
 * @stable ICU 2.4
 */
#define U8_IS_TRAIL(c) ((int8_t)(c)<-0x40)

/**
 * Get a code point from a string at a code point boundary offset,
 * and advance the offset to the next code point boundary.
 * (Post-incrementing forward iteration.)
 * "Unsafe" macro, assumes well-formed UTF-8.
 *
 * The offset may point to the lead byte of a multi-byte sequence,
 * in which case the macro will read the whole sequence.
 * The result is undefined if the offset points to a trail byte
 * or an illegal UTF-8 sequence.
 *
 * @param s const uint8_t * string
 * @param i string offset
 * @param c output UChar32 variable
 * @see U8_NEXT
 * @stable ICU 2.4
 */
#define U8_NEXT_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(uint8_t)(s)[(i)++]; \
    if(!U8_IS_SINGLE(c)) { \
        if((c)<0xe0) { \
            (c)=(((c)&0x1f)<<6)|((s)[(i)++]&0x3f); \
        } else if((c)<0xf0) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(::icu_utf::UChar)(((c)<<12)|(((s)[i]&0x3f)<<6)|((s)[(i)+1]&0x3f)); \
            (i)+=2; \
        } else { \
            (c)=(((c)&7)<<18)|(((s)[i]&0x3f)<<12)|(((s)[(i)+1]&0x3f)<<6)|((s)[(i)+2]&0x3f); \
            (i)+=3; \
        } \
    } \
} UPRV_BLOCK_MACRO_END

/**
 * Get a code point from a string at a code point boundary offset,
 * and advance the offset to the next code point boundary.
 * (Post-incrementing forward iteration.)
 * "Safe" macro, checks for illegal sequences and for string boundaries.
 *
 * The length can be negative for a NUL-terminated string.
 *
 * The offset may point to the lead byte of a multi-byte sequence,
 * in which case the macro will read the whole sequence.
 * If the offset points to a trail byte or an illegal UTF-8 sequence, then
 * c is set to U+FFFD.
 *
 * This macro does not distinguish between a real U+FFFD in the text
 * and U+FFFD returned for an ill-formed sequence.
 * Use U8_NEXT() if that distinction is important.
 *
 * @param s const uint8_t * string
 * @param i int32_t string offset, must be i<length
 * @param length int32_t string length
 * @param c output UChar32 variable, set to U+FFFD in case of an error
 * @see U8_NEXT
 * @stable ICU 51
 */
#define U8_NEXT_OR_FFFD(s, i, length, c) U8_INTERNAL_NEXT_OR_SUB(s, i, length, c, 0xfffd)

/** @internal */
#define U8_INTERNAL_NEXT_OR_SUB(s, i, length, c, sub) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(uint8_t)(s)[(i)++]; \
    if(!U8_IS_SINGLE(c)) { \
        uint8_t __t = 0; \
        if((i)!=(length) && \
            /* fetch/validate/assemble all but last trail byte */ \
            ((c)>=0xe0 ? \
                ((c)<0xf0 ?  /* U+0800..U+FFFF except surrogates */ \
                    U8_LEAD3_T1_BITS[(c)&=0xf]&(1<<((__t=(s)[i])>>5)) && \
                    (__t&=0x3f, 1) \
                :  /* U+10000..U+10FFFF */ \
                    ((c)-=0xf0)<=4 && \
                    U8_LEAD4_T1_BITS[(__t=(s)[i])>>4]&(1<<(c)) && \
                    ((c)=((c)<<6)|(__t&0x3f), ++(i)!=(length)) && \
                    (__t=(s)[i]-0x80)<=0x3f) && \
                /* valid second-to-last trail byte */ \
                ((c)=((c)<<6)|__t, ++(i)!=(length)) \
            :  /* U+0080..U+07FF */ \
                (c)>=0xc2 && ((c)&=0x1f, 1)) && \
            /* last trail byte */ \
            (__t=(s)[i]-0x80)<=0x3f && \
            ((c)=((c)<<6)|__t, ++(i), 1)) { \
        } else { \
            (c)=(sub);  /* ill-formed*/ \
        } \
    } \
} UPRV_BLOCK_MACRO_END

/**
 * Append a code point to a string, overwriting 1 to 4 bytes.
 * The offset points to the current end of the string contents
 * and is advanced (post-increment).
 * "Unsafe" macro, assumes a valid code point and sufficient space in the string.
 * Otherwise, the result is undefined.
 *
 * @param s const uint8_t * string buffer
 * @param i string offset
 * @param c code point to append
 * @see U8_APPEND
 * @stable ICU 2.4
 */
#define U8_APPEND_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    uint32_t __uc=(c); \
    if(__uc<=0x7f) { \
        (s)[(i)++]=(uint8_t)__uc; \
    } else { \
        if(__uc<=0x7ff) { \
            (s)[(i)++]=(uint8_t)((__uc>>6)|0xc0); \
        } else { \
            if(__uc<=0xffff) { \
                (s)[(i)++]=(uint8_t)((__uc>>12)|0xe0); \
            } else { \
                (s)[(i)++]=(uint8_t)((__uc>>18)|0xf0); \
                (s)[(i)++]=(uint8_t)(((__uc>>12)&0x3f)|0x80); \
            } \
            (s)[(i)++]=(uint8_t)(((__uc>>6)&0x3f)|0x80); \
        } \
        (s)[(i)++]=(uint8_t)((__uc&0x3f)|0x80); \
    } \
} UPRV_BLOCK_MACRO_END

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// source/common/unicode/utf16.h
// ─────────────────────────────────────────────────────────────────────────────────────────────────

/**
 * Is this code unit a lead surrogate (U+d800..U+dbff)?
 * @param c 16-bit code unit
 * @return true or false
 * @stable ICU 2.4
 */
#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)

/**
 * Is this code unit a trail surrogate (U+dc00..U+dfff)?
 * @param c 16-bit code unit
 * @return true or false
 * @stable ICU 2.4
 */
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

/**
 * Is this code unit a surrogate (U+d800..U+dfff)?
 * @param c 16-bit code unit
 * @return true or false
 * @stable ICU 2.4
 */
#define U16_IS_SURROGATE(c) U_IS_SURROGATE(c)

/**
 * Assuming c is a surrogate code point (U16_IS_SURROGATE(c)),
 * is it a lead surrogate?
 * @param c 16-bit code unit
 * @return true or false
 * @stable ICU 2.4
 */
#define U16_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)

/**
 * Helper constant for U16_GET_SUPPLEMENTARY.
 * @internal
 */
#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)

/**
 * Get a supplementary code point value (U+10000..U+10ffff)
 * from its lead and trail surrogates.
 * The result is undefined if the input values are not
 * lead and trail surrogates.
 *
 * @param lead lead surrogate (U+d800..U+dbff)
 * @param trail trail surrogate (U+dc00..U+dfff)
 * @return supplementary code point (U+10000..U+10ffff)
 * @stable ICU 2.4
 */
#define U16_GET_SUPPLEMENTARY(lead, trail) \
    (((::icu_utf::UChar32)(lead)<<10UL)+(::icu_utf::UChar32)(trail)-U16_SURROGATE_OFFSET)

/**
 * Get a code point from a string at a code point boundary offset,
 * and advance the offset to the next code point boundary.
 * (Post-incrementing forward iteration.)
 * "Unsafe" macro, assumes well-formed UTF-16.
 *
 * The offset may point to the lead surrogate unit
 * for a supplementary code point, in which case the macro will read
 * the following trail surrogate as well.
 * If the offset points to a trail surrogate, then that itself
 * will be returned as the code point.
 * The result is undefined if the offset points to a single, unpaired lead surrogate.
 *
 * @param s const UChar * string
 * @param i string offset
 * @param c output UChar32 variable
 * @see U16_NEXT
 * @stable ICU 2.4
 */
#define U16_NEXT_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { \
        (c)=U16_GET_SUPPLEMENTARY((c), (s)[(i)++]); \
    } \
} UPRV_BLOCK_MACRO_END

/**
 * Get a code point from a string at a code point boundary offset,
 * and advance the offset to the next code point boundary.
 * (Post-incrementing forward iteration.)
 * "Safe" macro, handles unpaired surrogates and checks for string boundaries.
 *
 * The length can be negative for a NUL-terminated string.
 *
 * The offset may point to the lead surrogate unit
 * for a supplementary code point, in which case the macro will read
 * the following trail surrogate as well.
 * If the offset points to a trail surrogate or
 * to a single, unpaired lead surrogate, then c is set to U+FFFD.
 *
 * @param s const UChar * string
 * @param i string offset, must be i<length
 * @param length string length
 * @param c output UChar32 variable
 * @see U16_NEXT_UNSAFE
 * @stable ICU 60
 */
#define U16_NEXT_OR_FFFD(s, i, length, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(s)[(i)++]; \
    if(U16_IS_SURROGATE(c)) { \
        uint16_t __c2; \
        if(U16_IS_SURROGATE_LEAD(c) && (i)!=(length) && U16_IS_TRAIL(__c2=(s)[(i)])) { \
            ++(i); \
            (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
        } else { \
            (c)=0xfffd; \
        } \
    } \
} UPRV_BLOCK_MACRO_END

/**
 * Append a code point to a string, overwriting 1 or 2 code units.
 * The offset points to the current end of the string contents
 * and is advanced (post-increment).
 * "Unsafe" macro, assumes a valid code point and sufficient space in the string.
 * Otherwise, the result is undefined.
 *
 * @param s const UChar * string buffer
 * @param i string offset
 * @param c code point to append
 * @see U16_APPEND
 * @stable ICU 2.4
 */
#define U16_APPEND_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    if((uint32_t)(c)<=0xffff) { \
        (s)[(i)++]=(uint16_t)(c); \
    } else { \
        (s)[(i)++]=(uint16_t)(((c)>>10)+0xd7c0); \
        (s)[(i)++]=(uint16_t)(((c)&0x3ff)|0xdc00); \
    } \
} UPRV_BLOCK_MACRO_END

// clang-format on

}  // namespace icu_utf
