#pragma once

#include "build/build_config.h"
#include <string>
#include <string_view>

#if BUILDFLAG(IS_MAC)
#include "base/apple/scoped_cftyperef.h"
#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
@class NSString;
#endif
#endif

namespace base {

#if BUILDFLAG(IS_WIN)

inline wchar_t* as_writable_wcstr(char16_t* str) { return reinterpret_cast<wchar_t*>(str); }

inline wchar_t* as_writable_wcstr(std::u16string& str) {
    return reinterpret_cast<wchar_t*>(data(str));
}

inline const wchar_t* as_wcstr(const char16_t* str) {
    return reinterpret_cast<const wchar_t*>(str);
}

inline const wchar_t* as_wcstr(std::u16string_view str) {
    return reinterpret_cast<const wchar_t*>(str.data());
}

inline std::wstring utf16_to_wide(std::u16string_view utf16) {
    return std::wstring(utf16.begin(), utf16.end());
}

inline std::u16string wide_to_utf16(std::wstring_view wide) {
    return std::u16string(wide.begin(), wide.end());
}

#elif BUILDFLAG(IS_MAC)

// Converts a string to a CFStringRef. Returns null on failure.
[[nodiscard]] apple::ScopedCFTypeRef<CFStringRef> utf8_to_cfstring(std::string_view utf8);
[[nodiscard]] apple::ScopedCFTypeRef<CFStringRef> utf16_to_cfstring(std::u16string_view utf16);

// Converts a CFStringRef to a string. Returns an empty string on failure. It is not valid to call
// these with a null `ref`.
[[nodiscard]] std::string cfstring_to_utf8(CFStringRef ref);
[[nodiscard]] std::u16string cfstring_to_utf16(CFStringRef ref);

#ifdef __OBJC__
// Converts a string to an autoreleased NSString. Returns nil on failure.
[[nodiscard]] NSString* utf8_to_nsstring(std::string_view utf8);
[[nodiscard]] NSString* utf16_to_nsstring(std::u16string_view utf16);

// Converts an NSString to a string. Returns an empty string on failure or if `ref` is nil.
[[nodiscard]] std::string nsstring_to_utf8(NSString* ref);
[[nodiscard]] std::u16string nsstring_to_utf16(NSString* ref);
#endif

#endif

}  // namespace base
