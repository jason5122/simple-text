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
}  // namespace

int main() {
    auto text = base::sys_utf8_to_cfstring_ref("Sphinx of black quartz, judge my vow. 0123456789");
    auto family = base::sys_utf8_to_cfstring_ref("Source Code Pro");
    CGFloat font_size = 16;
    const char* out_path = "out.png";

    constexpr size_t width = 1500;
    constexpr size_t height = 100;
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

    auto snap = [scale](CGFloat y) { return (std::round(y * scale - 0.5) + 0.5) / scale; };
    // auto snap = [scale](CGFloat y) { return (std::floor(y * scale) + 0.5) / scale; };
    spdlog::info("descent = {} -> {}", descent, snap(descent));
    spdlog::info("line_width = {} -> {}", line_width, std::ceil(line_width));

    CGContextSetTextPosition(ctx.get(), 0, snap(descent));
    CTLineDraw(line.get(), ctx.get());

    // Baseline.
    CGContextSetRGBStrokeColor(ctx.get(), 1, 0, 0, 1);
    CGContextSetLineWidth(ctx.get(), 1.0 / scale);
    CGContextMoveToPoint(ctx.get(), 0, snap(descent));
    CGContextAddLineToPoint(ctx.get(), std::ceil(line_width), snap(descent));
    CGContextStrokePath(ctx.get());

    auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    write_png(out_path, img.get());
}
