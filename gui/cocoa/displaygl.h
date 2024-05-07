#pragma once

#include "util/not_copyable_or_movable.h"

struct _CGLContextObject;
typedef _CGLContextObject* CGLContextObj;

struct _CGLPixelFormatObject;
typedef _CGLPixelFormatObject* CGLPixelFormatObj;

class DisplayGL {
public:
    NOT_COPYABLE(DisplayGL)
    NOT_MOVABLE(DisplayGL)
    DisplayGL();
    ~DisplayGL();

    bool initialize();

    // private:
    CGLContextObj mContext;
    CGLPixelFormatObj mPixelFormat;
};
