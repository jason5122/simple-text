#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/strings/sys_string_conversions.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColor;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

namespace {

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

void draw_text(CGContextRef ctx, CTLineRef line, CTFontRef, CGFloat descent) {
    CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
    CGContextSetTextDrawingMode(ctx, kCGTextFill);

    CGContextSetTextPosition(ctx, 0, descent);
    CTLineDraw(line, ctx);
}

// Identical to `draw_text`!
void draw_text2(CGContextRef ctx, CTLineRef line, CTFontRef font, CGFloat descent) {
    CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);

    CGPoint line_origin = {0, descent};
    CFArrayRef runs = CTLineGetGlyphRuns(line);
    CFIndex run_count = CFArrayGetCount(runs);

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        CFIndex n = CTRunGetGlyphCount(run);
        if (n <= 0) continue;

        std::vector<CGGlyph> glyphs(n);
        std::vector<CGPoint> positions(n);

        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs.data());
        CTRunGetPositions(run, CFRangeMake(0, 0), positions.data());

        // Run attributes can override the font. Use the run font if present.
        CTFontRef run_font = font;
        CFDictionaryRef run_attrs = CTRunGetAttributes(run);
        if (run_attrs) {
            auto v = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (v) run_font = (CTFontRef)v;
        }

        // Apply line origin to each glyph position (positions are relative to line origin).
        for (CFIndex i = 0; i < n; i++) {
            positions[i].x += line_origin.x;
            positions[i].y += line_origin.y;
        }
        CTFontDrawGlyphs(run_font, glyphs.data(), positions.data(), n, ctx);
    }
}

struct GlyphBitmap {
    size_t width;
    size_t height;
    size_t bytes_per_pixel;
    int bearing_x;
    int bearing_y;
    std::vector<uint8_t> pixels;
};

GlyphBitmap rasterize(CTFontRef font, CGGlyph glyph, int scale) {
    CGRect bounds =
        CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, nullptr, 1);
    CGFloat pad = 1.0 / scale;
    bounds = CGRectInset(bounds, -pad, -pad);

    // size_t width = std::ceil(bounds.size.width * scale);
    // size_t height = std::ceil(bounds.size.height * scale);
    int x0 = (int)std::floor(bounds.origin.x * scale);
    int y0 = (int)std::floor(bounds.origin.y * scale);
    int x1 = (int)std::ceil((bounds.origin.x + bounds.size.width) * scale);
    int y1 = (int)std::ceil((bounds.origin.y + bounds.size.height) * scale);
    size_t width = (size_t)std::max(0, x1 - x0);
    size_t height = (size_t)std::max(0, y1 - y0);
    int bearing_x = x0;
    int bearing_y = y0;

    size_t bytes_per_pixel = 4;
    size_t bytes_per_row = width * bytes_per_pixel;
    std::vector<uint8_t> pixels(height * bytes_per_row);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), width, height, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetRGBFillColor(ctx.get(), 0, 0, 0, 1);
    CGContextScaleCTM(ctx.get(), scale, scale);
    CGContextTranslateCTM(ctx.get(), -bearing_x / scale, -bearing_y / scale);

    CGPoint pos = {0, 0};
    CTFontDrawGlyphs(font, &glyph, &pos, 1, ctx.get());

    return {
        .width = width,
        .height = height,
        .bytes_per_pixel = bytes_per_pixel,
        .bearing_x = bearing_x,
        .bearing_y = bearing_y,
        .pixels = std::move(pixels),
    };
}

void blit_glyph_bitmap(CGContextRef ctx, const GlyphBitmap& bitmap, const CGPoint& origin) {
    auto& [width, height, bytes_per_pixel, bearing_x, bearing_y, pixels] = bitmap;
    size_t bytes_per_row = width * bytes_per_pixel;

    auto provider = ScopedCFTypeRef<CGDataProviderRef>(
        CGDataProviderCreateWithData(nullptr, pixels.data(), height * bytes_per_row, nullptr));
    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto img = ScopedCFTypeRef<CGImageRef>(
        CGImageCreate(width, height, 8, 32, bytes_per_row, cs.get(),
                      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host, provider.get(),
                      nullptr, true, kCGRenderingIntentDefault));

    CGContextSaveGState(ctx);
    constexpr int scale = 2;
    CGFloat bx = (CGFloat)bearing_x / (CGFloat)scale;
    CGFloat by = (CGFloat)bearing_y / (CGFloat)scale;
    CGRect rect = {
        .origin =
            {
                .x = origin.x + bx,
                .y = origin.y + by,
            },
        .size =
            {
                .width = (CGFloat)width / (CGFloat)scale,
                .height = (CGFloat)height / (CGFloat)scale,
            },
    };
    CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
    CGContextDrawImage(ctx, rect, img.get());
    CGContextRestoreGState(ctx);
}

// Identical to `draw_text`!
void draw_text3(CGContextRef ctx, CTLineRef line, CTFontRef font, CGFloat descent) {
    CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);

    CGPoint line_origin = {0, descent};
    CFArrayRef runs = CTLineGetGlyphRuns(line);
    CFIndex run_count = CFArrayGetCount(runs);

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        CFIndex n = CTRunGetGlyphCount(run);
        if (n <= 0) continue;

        std::vector<CGGlyph> glyphs(n);
        std::vector<CGPoint> positions(n);

        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs.data());
        CTRunGetPositions(run, CFRangeMake(0, 0), positions.data());

        // Run attributes can override the font. Use the run font if present.
        CTFontRef run_font = font;
        CFDictionaryRef run_attrs = CTRunGetAttributes(run);
        if (run_attrs) {
            auto v = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (v) run_font = (CTFontRef)v;
        }

        for (CFIndex i = 0; i < n; i++) {
            auto pos = positions[i];
            auto glyph = glyphs[i];
            pos.x += line_origin.x;
            pos.y += line_origin.y;
            // CTFontDrawGlyphs(run_font, &glyph, &pos, 1, ctx);

            constexpr size_t scale = 2;
            auto bitmap = rasterize(run_font, glyph, scale);
            blit_glyph_bitmap(ctx, bitmap, pos);
        }
    }
}

}  // namespace

int main() {
    auto text =
        base::sys_utf8_to_cfstring_ref("Sphinx of black quartz, judge my vow 😀. 0123456789");
    auto family = base::sys_utf8_to_cfstring_ref("Source Code Pro");
    CGFloat font_size = 16;
    const char* out_path = "out.png";

    constexpr size_t width = 1000;
    constexpr size_t height = 200;
    constexpr size_t scale = 2;

    constexpr size_t bytes_per_pixel = 4;
    constexpr size_t bytes_per_row = width * bytes_per_pixel;
    std::vector<uint8_t> pixels(height * bytes_per_row);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), width, height, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetRGBFillColor(ctx.get(), 1, 1, 1, 1);
    CGContextFillRect(ctx.get(), CGRectMake(0, 0, width, height));
    CGContextScaleCTM(ctx.get(), scale, scale);

    auto font = ScopedCFTypeRef<CTFontRef>(CTFontCreateWithName(family.get(), font_size, nullptr));
    const void* keys[] = {kCTFontAttributeName};
    const void* vals[] = {font.get()};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 1, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    auto line = ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));

    CGFloat ascent = 0, descent = 0, leading = 0;
    double line_width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);

    // draw_text(ctx.get(), line.get(), font.get(), descent);
    // draw_text2(ctx.get(), line.get(), font.get(), descent);
    draw_text3(ctx.get(), line.get(), font.get(), descent);

    // Debug.
    auto snap = [scale](CGFloat y) { return (std::floor(y * scale) + 0.5) / scale; };
    draw_line(ctx.get(), {1, 0, 0}, scale, {0, snap(descent)}, {ceil(line_width), snap(descent)});
    draw_line(ctx.get(), {0, 1, 0}, scale, {0, snap(ascent)}, {ceil(line_width), snap(ascent)});
    draw_line(ctx.get(), {0, 0, 1}, scale, {0, snap(0)}, {ceil(line_width), snap(0)});

    auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    write_png(out_path, img.get());
}
