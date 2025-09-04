#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include <Foundation/Foundation.h>
#include <vector>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;

namespace base {

namespace {

// Given a std::string_view `in` with an encoding specified by `in_encoding`,
// returns it as a CFStringRef. Returns null on failure.
template <typename CharT>
ScopedCFTypeRef<CFStringRef> string_piece_to_cfstring_with_encodings(
    std::basic_string_view<CharT> in, CFStringEncoding in_encoding) {
    const auto in_length = in.length();
    if (in_length == 0) {
        return ScopedCFTypeRef<CFStringRef>(CFSTR(""), OwnershipPolicy::kRetain);
    }

    return ScopedCFTypeRef<CFStringRef>(CFStringCreateWithBytes(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(in.data()),
        checked_cast<CFIndex>(in_length * sizeof(CharT)), in_encoding, false));
}

// Converts the supplied CFString into the specified encoding, and returns it as
// a C++ library string of the template type. Returns an empty string on
// failure.
//
// Do not assert in this function since it is used by the assertion code!
template <typename StringType>
StringType cfstring_to_string_with_encoding(CFStringRef cfstring, CFStringEncoding encoding) {
    CFIndex length = CFStringGetLength(cfstring);
    if (length == 0) {
        return StringType();
    }

    CFRange whole_string = CFRangeMake(0, length);
    CFIndex out_size;
    CFIndex converted = CFStringGetBytes(cfstring, whole_string, encoding,
                                         /*lossByte=*/0,
                                         /*isExternalRepresentation=*/false,
                                         /*buffer=*/nullptr,
                                         /*maxBufLen=*/0, &out_size);
    if (converted == 0 || out_size <= 0) {
        return StringType();
    }

    // `out_size` is the number of UInt8-sized units needed in the destination.
    // A buffer allocated as UInt8 units might not be properly aligned to
    // contain elements of StringType::value_type.  Use a container for the
    // proper value_type, and convert `out_size` by figuring the number of
    // value_type elements per UInt8.  Leave room for a NUL terminator.
    size_t elements =
        static_cast<size_t>(out_size) * sizeof(UInt8) / sizeof(typename StringType::value_type) +
        1;

    std::vector<typename StringType::value_type> out_buffer(elements);
    converted = CFStringGetBytes(cfstring, whole_string, encoding,
                                 /*lossByte=*/0,
                                 /*isExternalRepresentation=*/false,
                                 reinterpret_cast<UInt8*>(&out_buffer[0]), out_size,
                                 /*usedBufLen=*/nullptr);
    if (converted == 0) {
        return StringType();
    }

    out_buffer[elements - 1] = '\0';
    return StringType(&out_buffer[0], elements - 1);
}

}  // namespace

ScopedCFTypeRef<CFStringRef> sys_utf8_to_cfstring_ref(std::string_view utf8) {
    return string_piece_to_cfstring_with_encodings(utf8, kCFStringEncodingUTF8);
}

ScopedCFTypeRef<CFStringRef> sys_utf16_to_cfstring_ref(std::u16string_view utf16) {
    return string_piece_to_cfstring_with_encodings(utf16, kCFStringEncodingUTF16LE);
}

std::string sys_cfstring_ref_to_utf8(CFStringRef ref) {
    return cfstring_to_string_with_encoding<std::string>(ref, kCFStringEncodingUTF8);
}

std::u16string sys_cfstring_ref_to_utf16(CFStringRef ref) {
    return cfstring_to_string_with_encoding<std::u16string>(ref, kCFStringEncodingUTF16LE);
}

NSString* sys_utf8_to_nsstring(std::string_view utf8) {
    return (__bridge_transfer NSString*)sys_utf8_to_cfstring_ref(utf8).release();
}

NSString* sys_utf16_to_nsstring(std::u16string_view utf16) {
    return (__bridge_transfer NSString*)sys_utf16_to_cfstring_ref(utf16).release();
}

std::string sys_nsstring_to_utf8(NSString* nsstring) {
    if (!nsstring) return std::string();
    return sys_cfstring_ref_to_utf8((__bridge CFStringRef)nsstring);
}

std::u16string sys_nsstring_to_utf16(NSString* nsstring) {
    if (!nsstring) return std::u16string();
    return sys_cfstring_ref_to_utf16((__bridge CFStringRef)nsstring);
}

}  // namespace base
