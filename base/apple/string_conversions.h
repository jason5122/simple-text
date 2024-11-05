#pragma once

#include "base/apple/scoped_cftyperef.h"
#include <string_view>

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

inline NSString* StringToNSString(std::string_view str8) {
    return static_cast<NSString*>(StringToCFString(str8).release());
    // return static_cast<NSString*>(StringToCFStringNoCopy(str8).release());
}

}  // namespace base::apple
