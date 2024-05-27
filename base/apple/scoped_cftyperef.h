#pragma once

#include "base/apple/scoped_typeref.h"

#import <CoreFoundation/CoreFoundation.h>

namespace base::apple {

template <typename CFT> struct ScopedCFTypeRefTraits {
    static CFT InvalidValue() {
        return nullptr;
    }
    static CFT Retain(CFT object) {
        CFRetain(object);
        return object;
    }
    static void Release(CFT object) {
        CFRelease(object);
    }
};

template <typename CFT> using ScopedCFTypeRef = ScopedTypeRef<CFT, ScopedCFTypeRefTraits<CFT>>;

}
