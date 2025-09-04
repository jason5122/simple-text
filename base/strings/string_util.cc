#include "base/strings/string_util.h"
#include "base/third_party/icu/icu_utf.h"

// TODO: Clean up Chromium code.

namespace base {

namespace {

// Assuming that a pointer is the size of a "machine word", then
// uintptr_t is an integer type that is also a machine word.
using MachineWord = uintptr_t;

inline bool IsMachineWordAligned(const void* pointer) {
    return !(reinterpret_cast<MachineWord>(pointer) & (sizeof(MachineWord) - 1));
}

inline bool IsValidCharacter(base_icu::UChar32 code_point) {
    // Excludes non-characters (U+FDD0..U+FDEF, and all code points
    // ending in 0xFFFE or 0xFFFF) from the set of valid code points.
    // https://unicode.org/faq/private_use.html#nonchar1
    return (code_point >= 0 && code_point < 0xD800) ||
           (code_point >= 0xE000 && code_point < 0xFDD0) ||
           (code_point > 0xFDEF && code_point <= 0x10FFFF && (code_point & 0xFFFE) != 0xFFFE);
}

template <class Char>
bool DoIsStringASCII(const Char* characters, size_t length) {
    // Bitmasks to detect non ASCII characters for character sizes of 8, 16 and 32
    // bits.
    constexpr MachineWord NonASCIIMasks[] = {
        0, MachineWord(0x8080808080808080ULL), MachineWord(0xFF80FF80FF80FF80ULL),
        0, MachineWord(0xFFFFFF80FFFFFF80ULL),
    };

    if (!length) return true;
    constexpr MachineWord non_ascii_bit_mask = NonASCIIMasks[sizeof(Char)];
    static_assert(non_ascii_bit_mask, "Error: Invalid Mask");
    MachineWord all_char_bits = 0;
    const Char* end = characters + length;

    // Prologue: align the input.
    while (!IsMachineWordAligned(characters) && characters < end)
        all_char_bits |= static_cast<MachineWord>(*characters++);
    if (all_char_bits & non_ascii_bit_mask) return false;

    // Compare the values of CPU word size.
    constexpr size_t chars_per_word = sizeof(MachineWord) / sizeof(Char);
    constexpr int batch_count = 16;
    while (characters <= end - batch_count * chars_per_word) {
        all_char_bits = 0;
        for (int i = 0; i < batch_count; ++i) {
            all_char_bits |= *(reinterpret_cast<const MachineWord*>(characters));
            characters += chars_per_word;
        }
        if (all_char_bits & non_ascii_bit_mask) return false;
    }

    // Process the remaining words.
    all_char_bits = 0;
    while (characters <= end - chars_per_word) {
        all_char_bits |= *(reinterpret_cast<const MachineWord*>(characters));
        characters += chars_per_word;
    }

    // Process the remaining bytes.
    while (characters < end) all_char_bits |= static_cast<MachineWord>(*characters++);

    return !(all_char_bits & non_ascii_bit_mask);
}

template <bool (*Validator)(base_icu::UChar32)>
inline bool DoIsStringUTF8(std::string_view str) {
    const uint8_t* src = reinterpret_cast<const uint8_t*>(str.data());
    size_t src_len = str.length();
    size_t char_index = 0;

    while (char_index < src_len) {
        base_icu::UChar32 code_point;
        CBU8_NEXT(src, char_index, src_len, code_point);
        if (!Validator(code_point)) return false;
    }
    return true;
}

}  // namespace

bool is_string_utf8(std::string_view str) { return DoIsStringUTF8<IsValidCharacter>(str); }

bool is_string_ascii(std::string_view str) { return DoIsStringASCII(str.data(), str.length()); }

bool is_string_ascii(std::u16string_view str) { return DoIsStringASCII(str.data(), str.length()); }

}  // namespace base
