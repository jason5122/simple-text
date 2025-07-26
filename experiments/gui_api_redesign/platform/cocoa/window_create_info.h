#pragma once

#include "experiments/gui_api_redesign/platform/cocoa/gl_context.h"
#include "experiments/gui_api_redesign/platform/cocoa/gl_pixel_format.h"
#include "experiments/gui_api_redesign/window.h"

struct Window::CreateInfo {
    int width;
    int height;
    std::unique_ptr<GLContext> ctx;
    GLPixelFormat* pf;
};
