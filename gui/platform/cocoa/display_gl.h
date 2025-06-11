#pragma once

#include "base/apple/scoped_cgtyperef.h"
#include <memory>

using base::apple::ScopedTypeRef;

namespace gui {

class DisplayGL {
public:
    ~DisplayGL();
    static std::unique_ptr<DisplayGL> Create();

    CGLPixelFormatObj pixelFormat();
    CGLContextObj context();

private:
    ScopedTypeRef<CGLPixelFormatObj> pixel_format_;
    ScopedTypeRef<CGLContextObj> context_;

    DisplayGL(CGLPixelFormatObj pixel_format, CGLContextObj context);
};

}  // namespace gui
