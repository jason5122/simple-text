#include "experiments/harfbuzz_demo/shared.h"
#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int main() {
    std::println("HarfBuzz {} + Fontconfig/FreeType demo\n", hb_version_string());

    if (!FcInit()) {
        std::println(stderr, "Failed to initialize Fontconfig.");
        return 1;
    }

    // Discovery: match a family name to a concrete font file + face index.
    FcPattern* pattern = FcNameParse(reinterpret_cast<const FcChar8*>("DejaVu Sans"));
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern* matched = FcFontMatch(nullptr, pattern, &result);
    if (!matched) {
        std::println(stderr, "Fontconfig found no match.");
        FcPatternDestroy(pattern);
        return 1;
    }

    FcChar8* file = nullptr;
    FcPatternGetString(matched, FC_FILE, 0, &file);
    int face_index = 0;
    FcPatternGetInteger(matched, FC_INDEX, 0, &face_index);
    if (!file) {
        std::println(stderr, "Matched font has no file.");
        FcPatternDestroy(matched);
        FcPatternDestroy(pattern);
        return 1;
    }
    const char* file_path = reinterpret_cast<const char*>(file);
    std::println("Discovered font: {} (index {})\n", file_path, face_index);

    // The file is in hand, so give HarfBuzz the whole font as a blob.
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(file_path);
    if (!blob) {
        std::println(stderr, "Failed to read the font file.");
        FcPatternDestroy(matched);
        FcPatternDestroy(pattern);
        return 1;
    }
    hb_face_t* hb_face = hb_face_create(blob, static_cast<unsigned>(face_index));
    hb_blob_destroy(blob);  // the face retains it
    hb_font_t* font = MakeHbFont(hb_face);

    std::vector<ShapedGlyph> glyphs = ShapeAndPrint(font, "Rafting");

    // Rasterize with FreeType, compositing each glyph at its HarfBuzz position.
    FT_Library ft = nullptr;
    FT_Face ft_face = nullptr;
    if (FT_Init_FreeType(&ft) == 0 && FT_New_Face(ft, file_path, face_index, &ft_face) == 0) {
        FT_Set_Pixel_Sizes(ft_face, 0, static_cast<FT_UInt>(kEmPixels));
        int ascent = static_cast<int>(ft_face->size->metrics.ascender >> 6);
        int descent = static_cast<int>(-(ft_face->size->metrics.descender >> 6));

        double run_width = 0;
        for (const auto& g : glyphs) run_width += g.x_advance;
        const int pad = 3;
        int width = static_cast<int>(std::ceil(run_width)) + 2 * pad;
        int height = ascent + descent + 2 * pad;
        std::vector<uint8_t> coverage(static_cast<size_t>(width) * height, 0);

        double pen_x = pad;
        for (const auto& g : glyphs) {
            if (FT_Load_Glyph(ft_face, g.glyph_id, FT_LOAD_RENDER) == 0) {
                FT_GlyphSlot slot = ft_face->glyph;
                const FT_Bitmap& bmp = slot->bitmap;
                std::span<const uint8_t> src = UNSAFE_BUFFERS(
                    std::span(bmp.buffer, static_cast<size_t>(bmp.rows) * bmp.pitch));
                int origin_x = static_cast<int>(pen_x + g.x_offset) + slot->bitmap_left;
                int origin_y = pad + ascent - slot->bitmap_top - static_cast<int>(g.y_offset);
                for (int row = 0; row < static_cast<int>(bmp.rows); ++row) {
                    int y = origin_y + row;
                    if (y < 0 || y >= height) continue;
                    for (int col = 0; col < static_cast<int>(bmp.width); ++col) {
                        int x = origin_x + col;
                        if (x < 0 || x >= width) continue;
                        uint8_t v = src[static_cast<size_t>(row) * bmp.pitch + col];
                        uint8_t& dst = coverage[static_cast<size_t>(y) * width + x];
                        dst = std::max(dst, v);
                    }
                }
            }
            pen_x += g.x_advance;
        }
        std::println("Rasterized {}x{} bitmap (FreeType):", width, height);
        PrintBitmap(coverage, width, height, width);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft);
    } else {
        std::println(stderr, "Failed to initialize FreeType.");
    }

    hb_font_destroy(font);
    hb_face_destroy(hb_face);
    FcPatternDestroy(matched);
    FcPatternDestroy(pattern);
    return 0;
}
