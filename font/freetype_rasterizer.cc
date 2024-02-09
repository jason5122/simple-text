#include "freetype_rasterizer.h"
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

bool FreeTypeRasterizer::setup() {
    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    FT_Face face;
    error = FT_New_Face(ft, "otf/SourceCodePro-Regular.ttf", 0, &face);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    FT_UInt glyph_index = FT_Get_Char_Index(face, 'a');

    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error != FT_Err_Ok) {
        std::cerr << "FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    unsigned char* buffer = face->glyph->bitmap.buffer;
    unsigned int rows = face->glyph->bitmap.rows;
    unsigned int width = face->glyph->bitmap.width;
    int pitch = face->glyph->bitmap.pitch;

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < width; j++) {
            unsigned char pixel_brightness = buffer[i * pitch + j];

            if (pixel_brightness > 169) {
                fprintf(stderr, "*");
            } else if (pixel_brightness > 84) {
                fprintf(stderr, ".");
            } else {
                fprintf(stderr, " ");
            }
        }
        fprintf(stderr, "\n");
    }

    return true;
}
