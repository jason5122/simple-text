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

void draw_text(CGContextRef ctx,
               const font::ShapedLine& shaped,
               size_t line_index,
               double scale,
               bool use_subpixel_positioning) {
    const double ascent = std::ceil(shaped.ascent);
    const double line_height = ascent + std::ceil(shaped.descent) + std::ceil(shaped.leading);
    const double baseline_y = ascent + line_height * line_index;

    font::GlyphRasterizer rasterizer;
    for (const auto& run : shaped.runs) {
        for (const auto& g : run.glyphs) {
            double glyph_x = g.x_offset;               // points
            double glyph_y = baseline_y + g.y_offset;  // points

            // At <=16pt Sublime positions glyphs with 6-phase horizontal sub-pixel precision: pick
            // phase = floor(frac(device_x) * 6), render the glyph shifted by phase*scale/6 device
            // px (6 phases span one point), and snap the bitmap to the whole pixel below. Above
            // the gate it snaps to the nearest whole pixel.
            const double device_x_f = glyph_x * scale;
            long device_x = std::lround(device_x_f);
            double subpixel_x = 0.0;
            if (use_subpixel_positioning) {
                device_x = (long)std::floor(device_x_f);
                int phase = (int)std::floor((device_x_f - device_x) * 6.0);
                subpixel_x = phase * scale / 6.0;
            }

            font::GlyphBitmap bmp = rasterizer.rasterize(run.font, g.glyph_id, scale, subpixel_x);
            if (bmp.empty()) continue;

            // +2 / +124 are debug margins to line up with the Sublime capture.
            long dst_x = device_x + bmp.bearing_x + 2;
            long dst_y = std::lround(glyph_y * scale) + bmp.bearing_y + 124;

            BitmapView img = {
                .pixels = bmp.pixels,
                .width = bmp.width,
                .height = bmp.height,
            };
            blit_pixels(ctx, img, dst_x, dst_y);
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string> lines = {
        "",
        "",
        "",
        "Sphinx of black quartz, judge my vow!",
        "The quick brown fox jumps over the lazy dog. 你好",
        "",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
    };
    auto family = "system";
    double font_size = 16;

    constexpr size_t width = 2000;
    constexpr size_t height = 1000;
    constexpr double scale = 2.0;

    font::FontDatabase db;
    auto face = db.match({family, font::Weight::Normal, font::Slant::Normal});
    if (!face) return 1;
    auto handle = db.create_font(*face, font_size);
    if (!handle) return 1;

    font::TextShaper shaper;

    const bool use_subpixel_positioning = font_size <= 16.0;
    auto ctx = create_context(width, height);
    for (size_t i = 0; i < lines.size(); i++) {
        auto shaped = shaper.shape(*handle, lines[i]);
        draw_text(ctx.get(), shaped, i, scale, use_subpixel_positioning);
    }
    show_window(ctx.get(), scale);

    // Debug.
    // auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    // write_png("out.png", img.get());
}
