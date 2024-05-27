#pragma once

#include "base/apple/scoped_cgtyperef.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

using base::apple::ScopedTypeRef;

class DisplayGL {
public:
    NOT_COPYABLE(DisplayGL)
    NOT_MOVABLE(DisplayGL)
    ~DisplayGL();
    static std::unique_ptr<DisplayGL> Create();

    CGLPixelFormatObj pixelFormat();
    CGLContextObj context();

private:
    ScopedTypeRef<CGLPixelFormatObj> pixel_format_;
    ScopedTypeRef<CGLContextObj> context_;

    DisplayGL(CGLPixelFormatObj pixel_format, CGLContextObj context);
};
