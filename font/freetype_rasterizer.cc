#include "freetype_rasterizer.h"
#include <iostream>
#include <vector>

bool FreeTypeRasterizer::setup(const char* font_path) {
    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    error = FT_New_Face(ft, font_path, 0, &face);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 32);

    float ascent = std::round(static_cast<float>(face->ascender) / 64);
    float descent = std::round(static_cast<float>(face->descender) / 64);
    float glyph_height = std::round(static_cast<float>(face->height) / 64);

    // FIXME: This is a hack to match Core Text's metrics.
    ascent *= 2;
    descent *= 2;
    glyph_height *= 2;

    float global_glyph_height = ascent - descent;

    this->line_height = std::max(glyph_height, global_glyph_height) + 2;
    this->descent = descent;

    return true;
}

RasterizedGlyph FreeTypeRasterizer::rasterizeUTF8(const char* utf8_str) {
    FT_Error error;

    FT_UInt glyph_index = FT_Get_Char_Index(face, utf8_str[0]);

    // TODO: Handle errors.
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
    }

    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
    }

    FT_Bitmap bitmap = face->glyph->bitmap;
    unsigned char* buffer = bitmap.buffer;
    unsigned int rows = bitmap.rows;
    unsigned int width = bitmap.width;
    int pitch = bitmap.pitch;
    FT_Pixel_Mode pixel_mode = static_cast<FT_Pixel_Mode>(bitmap.pixel_mode);

    std::vector<uint8_t> packed_buffer;
    if (pixel_mode == FT_PIXEL_MODE_GRAY) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < width; j++) {
                unsigned char pixel_brightness = buffer[i * pitch + j];
                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
            }
        }
    }
    // TODO: Support more pixel bitmap modes.
    else {
        std::cerr << "Pixel bitmap is unsupported!\n";
    }

    float advance = static_cast<float>(face->glyph->advance.x) / 64;
    int32_t top = face->glyph->bitmap_top;

    // TODO: Apply this transformation in glyph atlas, not in rasterizer.
    top -= descent;

    return RasterizedGlyph{
        false,
        face->glyph->bitmap_left,
        top,
        static_cast<int32_t>(width),
        static_cast<int32_t>(rows),
        advance,
        packed_buffer,
    };
}
