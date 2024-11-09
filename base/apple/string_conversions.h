#pragma once

#include "base/apple/scoped_cftyperef.h"
#include <string>
#include <vector>

// From //chromium/src/base/strings/sys_string_conversions_apple.mm.

namespace base::apple {

inline ScopedCFTypeRef<CFStringRef> StringToCFString(std::string_view str8) {
    if (str8.empty()) return CFSTR("");

    return CFStringCreateWithBytes(kCFAllocatorDefault,
                                   reinterpret_cast<const UInt8*>(str8.data()), str8.length(),
                                   kCFStringEncodingUTF8, false);
}

// TODO: Investigate if the "no copy" version does anything meaningful.
inline ScopedCFTypeRef<CFStringRef> StringToCFStringNoCopy(std::string_view str8) {
    if (str8.empty()) return CFSTR("");

    return CFStringCreateWithBytesNoCopy(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(str8.data()), str8.length(),
        kCFStringEncodingUTF8, false, kCFAllocatorNull);
}

inline std::string CFStringToString(CFStringRef cfstring) {
    CFIndex length = CFStringGetLength(cfstring);
    if (length == 0) {
        return std::string();
    }

    CFRange whole_string = CFRangeMake(0, length);
    CFIndex out_size;
    CFIndex converted = CFStringGetBytes(cfstring, whole_string, kCFStringEncodingUTF8,
                                         /*lossByte=*/0,
                                         /*isExternalRepresentation=*/false,
                                         /*buffer=*/nullptr,
                                         /*maxBufLen=*/0, &out_size);
    if (converted == 0 || out_size <= 0) {
        return std::string();
    }

    // `out_size` is the number of UInt8-sized units needed in the destination.
    // A buffer allocated as UInt8 units might not be properly aligned to
    // contain elements of std::string::value_type.  Use a container for the
    // proper value_type, and convert `out_size` by figuring the number of
    // value_type elements per UInt8.  Leave room for a NUL terminator.
    size_t elements = static_cast<size_t>(out_size) * sizeof(UInt8) / sizeof(char) + 1;

    std::vector<char> out_buffer(elements);
    converted = CFStringGetBytes(cfstring, whole_string, kCFStringEncodingUTF8,
                                 /*lossByte=*/0,
                                 /*isExternalRepresentation=*/false,
                                 reinterpret_cast<UInt8*>(&out_buffer[0]), out_size,
                                 /*usedBufLen=*/nullptr);
    if (converted == 0) {
        return std::string();
    }

    out_buffer[elements - 1] = '\0';
    return std::string(&out_buffer[0], elements - 1);
}

#ifdef __OBJC__
inline NSString* StringToNSString(std::string_view str8) {
    return static_cast<NSString*>(StringToCFStringNoCopy(str8).release());
}

inline std::string NSStringToString(NSString* nsstring) {
    if (!nsstring) return std::string();
    return CFStringToString(static_cast<CFStringRef>(nsstring));
}
#endif

}  // namespace base::apple
