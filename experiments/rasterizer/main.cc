#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/mac_helpers.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

namespace {

void draw_text(CGContextRef ctx, const font::ShapedLine& shaped) {
    constexpr int scale = 2;

    font::GlyphRasterizer rasterizer;
    for (const auto& run : shaped.runs) {
        for (const auto& g : run.glyphs) {
            auto pos_x = g.x_offset;
            auto pos_y = g.y_offset;
            pos_y += shaped.descent;

            double fx = pos_x * scale;
            double fy = pos_y * scale;
            double sub_x = fx - std::floor(fx);
            double sub_y = fy - std::floor(fy);
            sub_x = 0;
            sub_y = 0;
            font::GlyphBitmap bmp = rasterizer.rasterize(run.font, g.glyph, sub_x, sub_y, scale);

            ImageView img = {
                .data = bmp.pixels.data(),
                .width = bmp.width,
                .height = bmp.height,
                .stride = bmp.width * bmp.bytes_per_pixel,
            };
            CGRect rect = CGRectMake(pos_x + (double)bmp.bearing_x / scale,
                                     pos_y + (double)bmp.bearing_y / scale,
                                     (double)bmp.width / scale, (double)bmp.height / scale);
            draw_pixels(ctx, img, rect);
        }
    }
}

}  // namespace

int main() {
    auto text = "Sphinx of black quartz, judge my vow 😀. 0123456789";
    auto family = "Source Code Pro";
    double font_size = 16;
    auto out_path = "out.png";

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

    font::FontDatabase db;
    auto face = db.match({family, font::Weight::Normal, font::Slant::Normal});
    if (!face) return 1;
    auto handle = db.create_font(*face, font_size);
    if (!handle) return 1;

    font::TextShaper shaper;
    auto shaped = shaper.shape(*handle, text);
    auto& [runs, line_width, ascent, descent] = shaped;

    draw_text(ctx.get(), shaped);

    // Debug.
    auto snap = [scale](CGFloat y) { return (std::floor(y * scale) + 0.5) / scale; };
    draw_line(ctx.get(), {1, 0, 0}, scale, {0, snap(descent)}, {ceil(line_width), snap(descent)});
    draw_line(ctx.get(), {0, 1, 0}, scale, {0, snap(ascent)}, {ceil(line_width), snap(ascent)});
    draw_line(ctx.get(), {0, 0, 1}, scale, {0, snap(0)}, {ceil(line_width), snap(0)});

    auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    write_png(out_path, img.get());
}
