#include "base/compiler_specific.h"
#include <cmath>
#include <hb-ot.h>
#include <hb.h>
#include <print>
#include <span>
#include <vector>

// Em size in pixels; the raster surfaces below are unscaled so 1 pt/DIP == 1 px.
constexpr double kEmPixels = 24.0;

// One shaped glyph in pixel units, platform-neutral.
struct ShapedGlyph {
    uint32_t glyph_id;
    float x_offset;  // along the text direction
    float y_offset;  // up
    float x_advance;
};

// Wraps a face (which already has table access wired up) in an hb_font_t that
// uses HarfBuzz's native OpenType funcs -- no platform shaper. 26.6 pixel scale
// so advances come back in pixels.
hb_font_t* MakeHbFont(hb_face_t* face) {
    hb_font_t* font = hb_font_create(face);
    hb_ot_font_set_funcs(font);
    int scale = static_cast<int>(std::lround(kEmPixels * 64));
    hb_font_set_scale(font, scale, scale);
    return font;
}

// Shapes `text`, prints the glyph/advance table, and returns the glyphs in pixel
// units for the caller to rasterize. Feeding UTF-8 makes each cluster the input
// byte offset directly.
std::vector<ShapedGlyph> ShapeAndPrint(hb_font_t* font, const char* text) {
    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);  // infer script/direction/language
    hb_shape(font, buffer, nullptr, 0);

    unsigned count = hb_buffer_get_length(buffer);
    // HarfBuzz hands back parallel raw arrays; wrap them once as spans.
    auto* info_ptr = hb_buffer_get_glyph_infos(buffer, nullptr);
    auto* pos_ptr = hb_buffer_get_glyph_positions(buffer, nullptr);
    std::span<const hb_glyph_info_t> infos = UNSAFE_BUFFERS(std::span(info_ptr, count));
    std::span<const hb_glyph_position_t> positions = UNSAFE_BUFFERS(std::span(pos_ptr, count));

    std::println("Shaped \"{}\" into {} glyphs:", text, count);
    std::println("  {:<8} {:<8} {:<11} {:<8}", "cluster", "glyph", "x-advance", "x-offset");
    std::vector<ShapedGlyph> glyphs(count);
    for (unsigned i = 0; i < count; ++i) {
        std::println("  {:<8} {:<8} {:<11.2f} {:<8.2f}", infos[i].cluster, infos[i].codepoint,
                     positions[i].x_advance / 64.0, positions[i].x_offset / 64.0);
        glyphs[i] =
            ShapedGlyph{infos[i].codepoint, static_cast<float>(positions[i].x_offset / 64.0),
                        static_cast<float>(positions[i].y_offset / 64.0),
                        static_cast<float>(positions[i].x_advance / 64.0)};
    }
    std::println("");
    hb_buffer_destroy(buffer);
    return glyphs;
}

// Prints an 8-bit coverage bitmap (0 = background, 255 = full ink) as ASCII art,
// top row first. `pitch` is the row stride in bytes. Sub-sampled to stay small.
void PrintBitmap(std::span<const uint8_t> pixels, int width, int height, int pitch) {
    constexpr std::string_view kRamp = " .:-=+*#%@";  // light -> dark
    const int ramp_max = static_cast<int>(kRamp.size()) - 1;
    const int col_stride = std::max(1, (width + 79) / 80);
    const int row_stride = std::max(1, (height + 39) / 40);
    for (int y = 0; y < height; y += row_stride) {
        std::string line;
        for (int x = 0; x < width; x += col_stride) {
            int coverage = pixels[static_cast<size_t>(y) * pitch + x];
            line.push_back(kRamp[coverage * ramp_max / 255]);
        }
        std::println("{}", line);
    }
}
