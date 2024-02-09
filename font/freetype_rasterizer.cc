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

    return true;
}

void FreeTypeRasterizer::rasterizeUTF8(const char* utf8_str) {
    FT_Error error;

    FT_UInt glyph_index = FT_Get_Char_Index(face, 'a');

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
        std::cerr << "Pixel bitmap is gray (FT_PIXEL_MODE_GRAY).\n";

        fprintf(stderr, "%ux%u = %u\n", rows, width, rows * width);
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

                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
                packed_buffer.push_back(pixel_brightness);
            }
            fprintf(stderr, "\n");
        }

        fprintf(stderr, "%zu\n", packed_buffer.size());
    } else {
        std::cerr << "Pixel bitmap is unsupported!\n";
    }
}
