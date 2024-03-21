#include "build/buildflag.h"
#include "freetype_rasterizer.h"
#include "util/file_util.h"
#include <chrono>
#include <cmath>
#include <freetype/freetype.h>
#include <hb.h>
#include <iostream>
#include <vector>

#include <ft2build.h>

class FreeTypeRasterizer::impl {
public:
    std::vector<std::pair<FT_Face, hb_font_t*>> font_fallback_list;

    std::pair<hb_codepoint_t, size_t> getGlyphIndex(const char* utf8_str);
};

FreeTypeRasterizer::FreeTypeRasterizer() : pimpl{new impl{}} {}

bool FreeTypeRasterizer::setup(std::string main_font_name, int font_size) {
    fs::path main_font_path = ResourcePath() / "fonts" / main_font_name;

    std::vector<const char*> font_paths;
    font_paths.push_back(main_font_path.c_str());
    // font_paths.push_back("/System/Library/Fonts/Apple Color Emoji.ttc");
    // font_paths.push_back("/System/Library/Fonts/Monaco.ttf");
    // font_paths.push_back("/System/Library/Fonts/NotoSansMyanmar.ttc");

    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Init_FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    for (const auto& path : font_paths) {
        FT_Face ft_face;
        error = FT_New_Face(ft, path, 0, &ft_face);
        if (error != FT_Err_Ok) {
            std::cerr << "FT_New_Face error: " << FT_Error_String(error) << '\n';
            return false;
        }
        FT_Set_Pixel_Sizes(ft_face, 0, font_size);

        hb_blob_t* hb_blob = hb_blob_create_from_file(path);
        hb_face_t* hb_face = hb_face_create(hb_blob, 0);
        hb_font_t* hb_font = hb_font_create(hb_face);
        hb_blob_destroy(hb_blob);
        hb_face_destroy(hb_face);

        pimpl->font_fallback_list.push_back({ft_face, hb_font});
    }

    FT_Face ft_main_face = pimpl->font_fallback_list[0].first;
    float ascent = std::round(static_cast<float>(ft_main_face->ascender) / 64);
    float descent = std::round(static_cast<float>(ft_main_face->descender) / 64);
    float glyph_height = std::round(static_cast<float>(ft_main_face->height) / 64);

    // #if IS_MAC
    // FIXME: This is a hack to match Core Text's metrics.
    ascent *= 2;
    descent *= 2;
    glyph_height *= 2;
    // #endif

    float global_glyph_height = ascent - descent;

    this->line_height = std::max(glyph_height, global_glyph_height) + 2;
    this->descent = descent;

    return true;
}

std::pair<hb_codepoint_t, size_t> FreeTypeRasterizer::impl::getGlyphIndex(const char* utf8_str) {
    for (size_t font_index = 0; font_index < font_fallback_list.size(); font_index++) {
        hb_font_t* hb_font = font_fallback_list[font_index].second;

        hb_buffer_t* buf;
        buf = hb_buffer_create();
        hb_buffer_add_utf8(buf, utf8_str, -1, 0, -1);

        hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
        hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
        hb_buffer_set_language(buf, hb_language_from_string("en", -1));

        hb_shape(hb_font, buf, NULL, 0);

        unsigned int glyph_count;
        hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);

        // for (unsigned int i = 0; i < glyph_count; i++) {
        //     hb_codepoint_t glyph_index = glyph_info[i].codepoint;
        //     fprintf(stderr, "{%d, %zu} %s\n", glyph_index, font_index, utf8_str);
        // }

        if (glyph_count > 0) {
            hb_codepoint_t glyph_index = glyph_info[0].codepoint;
            if (glyph_index > 0) {
                hb_buffer_destroy(buf);
                return {glyph_index, font_index};
            }
        }

        hb_buffer_destroy(buf);
    }
    return {0, 0};
}

RasterizedGlyph FreeTypeRasterizer::rasterizeUTF8(const char* utf8_str) {
    auto [glyph_index, font_index] = pimpl->getGlyphIndex(utf8_str);
    FT_Face ft_face = pimpl->font_fallback_list[font_index].first;

    // TODO: Handle errors.
    FT_Error error;
    error = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_COLOR);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Load_Glyph error: " << FT_Error_String(error) << '\n';
    }

    error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Render_Glyph error: " << FT_Error_String(error) << '\n';
    }

    FT_Bitmap bitmap = ft_face->glyph->bitmap;
    unsigned char* buffer = bitmap.buffer;
    unsigned int rows = bitmap.rows;
    unsigned int width = bitmap.width;
    int pitch = bitmap.pitch;
    FT_Pixel_Mode pixel_mode = static_cast<FT_Pixel_Mode>(bitmap.pixel_mode);

    bool colored = false;

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
    } else if (pixel_mode == FT_PIXEL_MODE_BGRA) {
        size_t pixels = rows * width * 4;
        packed_buffer.reserve(pixels);
        for (size_t i = 0; i < pixels; i += 4) {
            packed_buffer.push_back(buffer[i + 2]);
            packed_buffer.push_back(buffer[i + 1]);
            packed_buffer.push_back(buffer[i]);
            packed_buffer.push_back(buffer[i + 3]);
        }

        colored = true;
    }
    // TODO: Support more pixel bitmap modes.
    else {
        std::cerr << "Pixel bitmap is unsupported! " << pixel_mode << '\n';
    }

    float advance = static_cast<float>(ft_face->glyph->advance.x) / 64;
    int32_t top = ft_face->glyph->bitmap_top;

    // TODO: Apply this transformation in glyph atlas, not in rasterizer.
    top -= descent;

    return RasterizedGlyph{
        colored,
        ft_face->glyph->bitmap_left,
        top,
        static_cast<int32_t>(width),
        static_cast<int32_t>(rows),
        advance,
        packed_buffer,
    };
}

FreeTypeRasterizer::~FreeTypeRasterizer() {
    for (const auto& [ft_font, hb_font] : pimpl->font_fallback_list) {
        hb_font_destroy(hb_font);
    }
}
