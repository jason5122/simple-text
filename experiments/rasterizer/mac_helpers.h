#pragma once

#include "base/apple/scoped_cgtyperef.h"
#include <span>
#include <string_view>

struct BitmapView {
    std::span<const uint8_t> pixels;
    size_t width;
    size_t height;
};

// TODO: Make this take in a CGContext instead, and just create the CGImage in the function.
void write_png(std::string_view path, base::apple::ScopedCGImage image);

void blit_pixels(base::apple::ScopedCGContext ctx, const BitmapView& img, double x, double y);

void show_window(base::apple::ScopedCGContext ctx, double scale);

base::apple::ScopedCGContext create_context(size_t width, size_t height);
