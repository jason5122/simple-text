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

void draw_text(CGContextRef ctx, const font::ShapedLine& shaped, size_t line_index, double scale) {
    const double ascent = std::ceil(shaped.ascent);
    const double line_height = ascent + std::ceil(shaped.descent) + std::ceil(shaped.leading);
    const double baseline_y = ascent + line_height * line_index;

    font::GlyphRasterizer rasterizer;
    for (const auto& run : shaped.runs) {
        for (const auto& g : run.glyphs) {
            font::GlyphBitmap bmp = rasterizer.rasterize(run.font, g.glyph_id, scale);
            if (bmp.empty()) continue;

            // We don't do sub-pixel placement yet. ST does this at ~half-pixel phases.
            // TODO: Implement this when we move to OpenGL.
            double glyph_x = g.x_offset;               // points
            double glyph_y = baseline_y + g.y_offset;  // points
            long dst_x = std::lround(glyph_x * scale) + bmp.bearing_x;
            long dst_y = std::lround(glyph_y * scale) + bmp.bearing_y;

            // Debug use. Helps us line up with Sublime Text for pixel perfect comparison.
            dst_x += 2;
            dst_y += 124;

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

int main() {
    std::vector<std::string> lines = {
        "Sphinx of black quartz, judge my vow! 😀😀😀",
        "The quick brown fox jumps over the lazy dog. 你好",
        "",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        "",
        "=== ==> => !== != ==",
    };
    auto family = "Fira Code";
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

    auto ctx = create_context(width, height);
    for (size_t i = 0; i < lines.size(); i++) {
        auto shaped = shaper.shape(*handle, lines[i]);
        draw_text(ctx.get(), shaped, i, scale);
    }
    show_window(ctx.get(), scale);

    // Debug.
    // auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    // write_png("out.png", img.get());
}
