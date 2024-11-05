#pragma once

#import <Foundation/Foundation.h>

namespace base::apple {

// Taken from //chromium/src/base/apple/foundation_util.h.

// ObjCCast<>() and ObjCCastStrict<>() cast a basic id to a more
// specific (NSObject-derived) type. The compatibility of the passed
// object is found by checking if it's a kind of the requested type
// identifier. If the supplied object is not compatible with the
// requested return type, ObjCCast<>() returns nil and
// ObjCCastStrict<>() will DCHECK. Providing a nil pointer to either
// variant results in nil being returned without triggering any DCHECK.
//
// The strict variant is useful when retrieving a value from a
// collection which only has values of a specific type, e.g. an
// NSArray of NSStrings. The non-strict variant is useful when
// retrieving values from data that you can't fully control. For
// example, a plist read from disk may be beyond your exclusive
// control, so you'd only want to check that the values you retrieve
// from it are of the expected types, but not crash if they're not.
//
// Example usage:
// NSString* version = base::apple::ObjCCast<NSString>(
//     [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"]);
//
// NSString* str = base::apple::ObjCCastStrict<NSString>(
//     [ns_arr_of_ns_strs objectAtIndex:0]);
template <typename T>
T* ObjCCast(id objc_val) {
    if ([objc_val isKindOfClass:[T class]]) {
        return reinterpret_cast<T*>(objc_val);
    }
    return nil;
}

template <typename T>
T* ObjCCastStrict(id objc_val) {
    T* rv = ObjCCast<T>(objc_val);
    assert(objc_val == nil || rv);
    return rv;
}

}  // namespace base::apple
