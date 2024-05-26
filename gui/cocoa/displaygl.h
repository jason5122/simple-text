#pragma once

#include "base/apple/scoped_typeref.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

struct _CGLContextObject;
typedef _CGLContextObject* CGLContextObj;

struct _CGLPixelFormatObject;
typedef _CGLPixelFormatObject* CGLPixelFormatObj;

class DisplayGL {
public:
    NOT_COPYABLE(DisplayGL)
    NOT_MOVABLE(DisplayGL)
    ~DisplayGL();
    static std::unique_ptr<DisplayGL> Create();

    CGLPixelFormatObj pixelFormat();
    CGLContextObj context();

private:
    base::apple::ScopedTypeRef<CGLPixelFormatObj> pixel_format_;
    base::apple::ScopedTypeRef<CGLContextObj> context_;

    DisplayGL(CGLPixelFormatObj pixel_format, CGLContextObj context);
};
