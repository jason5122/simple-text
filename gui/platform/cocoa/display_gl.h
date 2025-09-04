#pragma once

#include "base/apple/scoped_cgtyperef.h"
#include <memory>

namespace gui {

class DisplayGL {
public:
    ~DisplayGL();
    static std::unique_ptr<DisplayGL> Create();

    CGLPixelFormatObj pixelFormat();
    CGLContextObj context();

private:
    base::apple::ScopedCGLPixelFormat pixel_format_;
    base::apple::ScopedCGLContext context_;

    DisplayGL(CGLPixelFormatObj pixel_format, CGLContextObj context);
};

}  // namespace gui
