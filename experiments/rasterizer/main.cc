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

void draw_line(CGContextRef ctx, Color color, int scale, CGPoint p1, CGPoint p2) {
    CGContextSetRGBStrokeColor(ctx, color.r, color.g, color.b, 1);
    CGContextSetLineWidth(ctx, 1.0 / scale);
    CGContextMoveToPoint(ctx, p1.x, p1.y);
    CGContextAddLineToPoint(ctx, p2.x, p2.y);
    CGContextStrokePath(ctx);
}
}  // namespace

int main() {
    auto text = base::sys_utf8_to_cfstring_ref("Sphinx of black quartz, judge my vow. 0123456789");
    auto family = base::sys_utf8_to_cfstring_ref("Source Code Pro");
    CGFloat font_size = 16;
    const char* out_path = "out.png";

    constexpr size_t width = 1000;
    constexpr size_t height = 200;
    constexpr size_t scale = 2;

    constexpr size_t bytes_per_pixel = 4;
    constexpr size_t bytes_per_row = width * bytes_per_pixel;
    std::vector<uint8_t> pixels(height * bytes_per_row, 0);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), width, height, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Host));

    CGContextSetRGBFillColor(ctx.get(), 1, 1, 1, 1);
    CGContextFillRect(ctx.get(), CGRectMake(0, 0, width, height));
    CGContextScaleCTM(ctx.get(), scale, scale);

    auto font = ScopedCFTypeRef<CTFontRef>(CTFontCreateWithName(family.get(), font_size, nullptr));
    auto black = ScopedCGColor(CGColorCreateGenericRGB(0, 0, 0, 1));
    const void* keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
    const void* vals[] = {font.get(), black.get()};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    auto line = ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));

    CGFloat ascent = 0, descent = 0, leading = 0;
    double line_width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);

    // Round.
    auto snap = [scale](CGFloat y) { return (std::floor(y * scale) + 0.5) / scale; };
    spdlog::info("descent = {} -> {}", descent, snap(descent));
    spdlog::info("line_width = {} -> {}", line_width, std::ceil(line_width));
    spdlog::info("snap(0) = {}", snap(0));
    ascent = snap(ascent);
    descent = snap(descent);
    line_width = std::ceil(line_width);
    CGFloat baseline = snap(0);

    CGContextSetTextPosition(ctx.get(), 0, descent);
    CTLineDraw(line.get(), ctx.get());

    // Debug.
    draw_line(ctx.get(), {1, 0, 0}, scale, {0, descent}, {line_width, descent});
    draw_line(ctx.get(), {0, 1, 0}, scale, {0, ascent}, {line_width, ascent});
    draw_line(ctx.get(), {0, 0, 1}, scale, {0, baseline}, {line_width, baseline});

    auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    write_png(out_path, img.get());
}
