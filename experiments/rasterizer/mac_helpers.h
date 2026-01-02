#pragma once

#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "experiments/rasterizer/font.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <spdlog/spdlog.h>
#include <string_view>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

void write_png(std::string_view path, CGImageRef image) {
    auto cf_path = base::sys_utf8_to_cfstring_ref(path);
    auto url = ScopedCFTypeRef<CFURLRef>(CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, cf_path.get(), kCFURLPOSIXPathStyle, false));

    auto dest = ScopedCFTypeRef<CGImageDestinationRef>(
        CGImageDestinationCreateWithURL(url.get(), CFSTR("public.png"), 1, nullptr));
    CGImageDestinationAddImage(dest.get(), image, nullptr);
    CGImageDestinationFinalize(dest.get());
}

struct Color {
    CGFloat r, g, b;
};

void draw_line(
    CGContextRef ctx, const Color& color, int scale, const CGPoint& p1, const CGPoint& p2) {
    CGContextSetRGBStrokeColor(ctx, color.r, color.g, color.b, 1);
    CGContextSetLineWidth(ctx, 1.0 / scale);
    CGContextMoveToPoint(ctx, p1.x, p1.y);
    CGContextAddLineToPoint(ctx, p2.x, p2.y);
    CGContextStrokePath(ctx);
}

void blit_bitmap(CGContextRef ctx, const font::GlyphBitmap& bm, int scale) {
    if (bm.width == 0 || bm.height == 0) return;

    size_t bytes_per_row = bm.width * bm.bytes_per_pixel;
    auto provider = ScopedCFTypeRef<CGDataProviderRef>(CGDataProviderCreateWithData(
        nullptr, bm.pixels.data(), bm.height * bytes_per_row, nullptr));
    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto img = ScopedCFTypeRef<CGImageRef>(
        CGImageCreate(bm.width, bm.height, 8, 32, bytes_per_row, cs.get(),
                      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host, provider.get(),
                      nullptr, true, kCGRenderingIntentDefault));

    // Destination rect in user space.
    CGRect rect = {
        .origin = {(CGFloat)bm.bearing_x / scale, (CGFloat)bm.bearing_y / scale},
        .size = {(CGFloat)bm.width / scale, (CGFloat)bm.height / scale},
    };

    CGContextDrawImage(ctx, rect, img.get());
}
