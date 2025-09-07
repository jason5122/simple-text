#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/icu/icu_utf.h"
#include "base/unicode/unicode.h"

// TODO: Clean up Chromium code.

namespace base {

namespace {

constexpr base_icu::UChar32 kErrorCodePoint = 0xFFFD;

// UnicodeAppendUnsafe --------------------------------------------------------
// Function overloads that write code_point to the output string. Output string
// has to have enough space for the codepoint.

// Convenience typedef that checks whether the passed in type is integral (i.e.
// bool, char, int or their extended versions) and is of the correct size.
template <typename Char, size_t N>
concept BitsAre = std::integral<Char> && CHAR_BIT * sizeof(Char) == N;

template <typename Char>
    requires(BitsAre<Char, 8>)
void UnicodeAppendUnsafe(Char* out, size_t* size, base_icu::UChar32 code_point) {
    CBU8_APPEND_UNSAFE(reinterpret_cast<uint8_t*>(out), *size, code_point);
}

template <typename Char>
    requires(BitsAre<Char, 16>)
void UnicodeAppendUnsafe(Char* out, size_t* size, base_icu::UChar32 code_point) {
    CBU16_APPEND_UNSAFE(out, *size, code_point);
}

template <typename Char>
    requires(BitsAre<Char, 32>)
void UnicodeAppendUnsafe(Char* out, size_t* size, base_icu::UChar32 code_point) {
    out[(*size)++] = static_cast<Char>(code_point);
}

// DoUTFConversion ------------------------------------------------------------
// Main driver of UTFConversion specialized for different Src encodings.
// dest has to have enough room for the converted text.

template <typename DestChar>
bool DoUTFConversion(const char* src, size_t src_len, DestChar* dest, size_t* dest_len) {
    bool success = true;

    for (size_t i = 0; i < src_len;) {
        base_icu::UChar32 code_point;
        CBU8_NEXT(reinterpret_cast<const uint8_t*>(src), i, src_len, code_point);

        if (!is_valid_codepoint(code_point)) {
            success = false;
            code_point = kErrorCodePoint;
        }

        UnicodeAppendUnsafe(dest, dest_len, code_point);
    }

    return success;
}

template <typename DestChar>
bool DoUTFConversion(const char16_t* src, size_t src_len, DestChar* dest, size_t* dest_len) {
    bool success = true;

    auto ConvertSingleChar = [&success](char16_t in) -> base_icu::UChar32 {
        if (!CBU16_IS_SINGLE(in) || !is_valid_codepoint(in)) {
            success = false;
            return kErrorCodePoint;
        }
        return in;
    };

    size_t i = 0;

    // Always have another symbol in order to avoid checking boundaries in the
    // middle of the surrogate pair.
    while (i + 1 < src_len) {
        base_icu::UChar32 code_point;

        if (CBU16_IS_LEAD(src[i]) && CBU16_IS_TRAIL(src[i + 1])) {
            code_point = CBU16_GET_SUPPLEMENTARY(src[i], src[i + 1]);
            if (!is_valid_codepoint(code_point)) {
                code_point = kErrorCodePoint;
                success = false;
            }
            i += 2;
        } else {
            code_point = ConvertSingleChar(src[i]);
            ++i;
        }

        UnicodeAppendUnsafe(dest, dest_len, code_point);
    }

    if (i < src_len) {
        UnicodeAppendUnsafe(dest, dest_len, ConvertSingleChar(src[i]));
    }

    return success;
}

// Size coefficient ----------------------------------------------------------
// The maximum number of codeunits in the destination encoding corresponding to
// one codeunit in the source encoding.

template <typename SrcChar, typename DestChar>
struct SizeCoefficient {
    static_assert(sizeof(SrcChar) < sizeof(DestChar),
                  "Default case: from a smaller encoding to the bigger one");

    // ASCII symbols are encoded by one codeunit in all encodings.
    static constexpr int value = 1;
};

template <>
struct SizeCoefficient<char16_t, char> {
    // One UTF-16 codeunit corresponds to at most 3 codeunits in UTF-8.
    static constexpr int value = 3;
};

template <typename SrcChar, typename DestChar>
constexpr int size_coefficient_v =
    SizeCoefficient<std::decay_t<SrcChar>, std::decay_t<DestChar>>::value;

// UTFConversion --------------------------------------------------------------
// Function template for generating all UTF conversions.

template <typename InputString, typename DestString>
bool UTFConversion(const InputString& src_str, DestString* dest_str) {
    if (is_string_ascii(src_str)) {
        dest_str->assign(src_str.begin(), src_str.end());
        return true;
    }

    dest_str->resize(
        src_str.length() *
        size_coefficient_v<typename InputString::value_type, typename DestString::value_type>);

    // Empty string is ASCII => it OK to call operator[].
    auto* dest = &(*dest_str)[0];

    // ICU requires 32 bit numbers.
    size_t src_len = src_str.length();
    size_t dest_len = 0;

    bool res = DoUTFConversion(src_str.data(), src_len, dest, &dest_len);

    dest_str->resize(dest_len);
    dest_str->shrink_to_fit();

    return res;
}

}  // namespace

// UTF16 <-> UTF8 --------------------------------------------------------------

std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string ret;
    // Ignore the success flag of this call. It will do the best it can for invalid input.
    UTFConversion(utf8, &ret);
    return ret;
}

std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string ret;
    // Ignore the success flag of this call, it will do the best it can for
    // invalid input, which is what we want here.
    UTFConversion(utf16, &ret);
    return ret;
}

}  // namespace base
