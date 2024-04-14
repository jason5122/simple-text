#pragma once

struct _CGLContextObject;
typedef _CGLContextObject* CGLContextObj;

struct _CGLPixelFormatObject;
typedef _CGLPixelFormatObject* CGLPixelFormatObj;

class DisplayGL {
public:
    DisplayGL();
    ~DisplayGL();

    bool initialize();

private:
    CGLContextObj mContext;
    CGLPixelFormatObj mPixelFormat;
};
