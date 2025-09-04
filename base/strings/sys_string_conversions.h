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
[[nodiscard]] std::string sys_wide_to_utf8(std::wstring_view wide);
[[nodiscard]] std::wstring sys_utf8_to_wide(std::string_view utf8);
#endif

#if BUILDFLAG(IS_MAC)

// Converts a string to a CFStringRef. Returns null on failure.
[[nodiscard]] apple::ScopedCFTypeRef<CFStringRef> sys_utf8_to_cfstring_ref(std::string_view utf8);
[[nodiscard]] apple::ScopedCFTypeRef<CFStringRef> sys_utf16_to_cfstring_ref(
    std::u16string_view utf16);

// Converts a CFStringRef to a string. Returns an empty string on failure. It is not valid to call
// these with a null `ref`.
[[nodiscard]] std::string sys_cfstring_ref_to_utf8(CFStringRef ref);
[[nodiscard]] std::u16string sys_cfstring_ref_to_utf16(CFStringRef ref);

#ifdef __OBJC__
// Converts a string to an autoreleased NSString. Returns nil on failure.
[[nodiscard]] NSString* sys_utf8_to_nsstring(std::string_view utf8);
[[nodiscard]] NSString* sys_utf16_to_nsstring(std::u16string_view utf16);

// Converts an NSString to a string. Returns an empty string on failure or if `ref` is nil.
[[nodiscard]] std::string sys_nsstring_to_utf8(NSString* ref);
[[nodiscard]] std::u16string sys_nsstring_to_utf16(NSString* ref);
#endif
#endif

}  // namespace base
