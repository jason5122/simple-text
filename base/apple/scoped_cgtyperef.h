#pragma once

#include "base/apple/scoped_typeref.h"
#include <CoreGraphics/CoreGraphics.h>
#include <OpenGL/OpenGL.h>

namespace base::apple {

template <>
struct ScopedTypeRefTraits<CGLContextObj> {
    static CGLContextObj InvalidValue() { return nullptr; }
    static CGLContextObj Retain(CGLContextObj object) { return CGLRetainContext(object); }
    static void Release(CGLContextObj object) { CGLReleaseContext(object); }
};

template <>
struct ScopedTypeRefTraits<CGLPixelFormatObj> {
    static CGLPixelFormatObj InvalidValue() { return nullptr; }
    static CGLPixelFormatObj Retain(CGLPixelFormatObj object) {
        return CGLRetainPixelFormat(object);
    }
    static void Release(CGLPixelFormatObj object) { CGLReleasePixelFormat(object); }
};

template <>
struct ScopedTypeRefTraits<CGColorSpaceRef> {
    static CGColorSpaceRef InvalidValue() { return nullptr; }
    static CGColorSpaceRef Retain(CGColorSpaceRef object) { return CGColorSpaceRetain(object); }
    static void Release(CGColorSpaceRef object) { CGColorSpaceRelease(object); }
};

template <>
struct ScopedTypeRefTraits<CGContextRef> {
    static CGContextRef InvalidValue() { return nullptr; }
    static CGContextRef Retain(CGContextRef object) { return CGContextRetain(object); }
    static void Release(CGContextRef object) { CGContextRelease(object); }
};

template <>
struct ScopedTypeRefTraits<CGImageRef> {
    static CGImageRef InvalidValue() { return nullptr; }
    static CGImageRef Retain(CGImageRef object) { return CGImageRetain(object); }
    static void Release(CGImageRef object) { CGImageRelease(object); }
};

template <>
struct ScopedTypeRefTraits<CGColorRef> {
    static CGColorRef InvalidValue() { return nullptr; }
    static CGColorRef Retain(CGColorRef object) { return CGColorRetain(object); }
    static void Release(CGColorRef object) { CGColorRelease(object); }
};

using ScopedCGLContext = ScopedTypeRef<CGLContextObj>;
using ScopedCGLPixelFormat = ScopedTypeRef<CGLPixelFormatObj>;
using ScopedCGColorSpace = ScopedTypeRef<CGColorSpaceRef>;
using ScopedCGContext = ScopedTypeRef<CGContextRef>;
using ScopedCGImage = ScopedTypeRef<CGImageRef>;
using ScopedCGColor = ScopedTypeRef<CGColorRef>;

}  // namespace base::apple
