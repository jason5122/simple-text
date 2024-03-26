#include "build/buildflag.h"
#include "rasterizer.h"
#include "util/file_util.h"
#include <cmath>
#include <freetype/freetype.h>
#include <hb-ft.h>
#include <hb.h>
#include <iostream>
#include <pango/pangocairo.h>
#include <vector>

#include <ft2build.h>

class FontRasterizer::impl {
public:
    hb_font_t* hb_font;
    PangoFont* pango_font;
    PangoContext* context;
    hb_codepoint_t getGlyphIndex(const char* utf8_str);
    PangoGlyph getPangoGlyph(const char* utf8_str);
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    pimpl->context = pango_font_map_create_context(font_map);

    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, "Source Code Pro");
    pango_font_description_set_size(desc, font_size * PANGO_SCALE);
    if (!desc) {
        std::cerr << "pango_font_description_from_string() error.\n";
        return false;
    }

    pimpl->pango_font = pango_font_map_load_font(font_map, pimpl->context, desc);
    if (!pimpl->pango_font) {
        std::cerr << "pango_font_map_load_font() error.\n";
        return false;
    }

    fs::path main_font_path = ResourcePath() / "fonts" / main_font_name;

    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_Init_FreeType error: " << FT_Error_String(error) << '\n';
        return false;
    }

    FT_Face ft_face;
    error = FT_New_Face(ft, main_font_path.c_str(), 0, &ft_face);
    if (error != FT_Err_Ok) {
        std::cerr << "FT_New_Face error: " << FT_Error_String(error) << '\n';
        return false;
    }
    FT_Set_Pixel_Sizes(ft_face, 0, font_size);

    pimpl->hb_font = hb_ft_font_create(ft_face, nullptr);

    float ascent = std::round(static_cast<float>(ft_face->ascender) / 64);
    float descent = std::round(static_cast<float>(ft_face->descender) / 64);
    float glyph_height = std::round(static_cast<float>(ft_face->height) / 64);

    // TODO: Multiply by scale factor instead of hard coding.
    ascent *= 2;
    descent *= 2;
    glyph_height *= 2;

    float global_glyph_height = ascent - descent;

    this->line_height = std::max(glyph_height, global_glyph_height) + 2;
    this->descent = descent;

    // PangoFontMetrics* metrics = pango_font_get_metrics(pimpl->pango_font, nullptr);
    // if (!metrics) {
    //     std::cerr << "pango_font_get_metrics() error.\n";
    //     return false;
    // }

    // int pango_ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
    // int pango_descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
    // int pango_height = pango_font_metrics_get_height(metrics) / PANGO_SCALE;

    // pango_ascent *= 2;
    // pango_descent *= 2;
    // pango_height *= 2;

    // int pango_glyph_height = pango_ascent + pango_descent;

    // std::cerr << "pango_ascent: " << pango_ascent << '\n';
    // std::cerr << "pango_descent: " << pango_descent << '\n';
    // std::cerr << "pango_height: " << pango_height << '\n';
    // std::cerr << "pango_glyph_height: " << pango_glyph_height << '\n';
    // std::cerr << "this->line_height: " << this->line_height << '\n';

    return true;
}

hb_codepoint_t FontRasterizer::impl::getGlyphIndex(const char* utf8_str) {
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
            return glyph_index;
        }
    }

    hb_buffer_destroy(buf);
    return 0;
}

PangoGlyph FontRasterizer::impl::getPangoGlyph(const char* utf8_str) {
    PangoAttrList* attrs = pango_attr_list_new();
    GList* items = pango_itemize(context, utf8_str, 0, strlen(utf8_str), attrs, nullptr);
    PangoItem* item = static_cast<PangoItem*>(items->data);
    PangoAnalysis analysis = item->analysis;

    PangoGlyphString* glyph_string = pango_glyph_string_new();
    pango_shape(utf8_str, strlen(utf8_str), &analysis, glyph_string);

    PangoGlyphInfo* glyphs = glyph_string->glyphs;
    for (int i = 0; i < glyph_string->num_glyphs; i++) {
        PangoGlyphInfo glyph_info = glyphs[i];
        return glyph_info.glyph;
    }
    return 0;
}

cairo_t* create_layout_context() {
    cairo_surface_t* temp_surface;
    cairo_t* context;

    temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    context = cairo_create(temp_surface);
    cairo_surface_destroy(temp_surface);

    return context;
}

cairo_t* create_cairo_context(int width, int height, int channels, cairo_surface_t** surf,
                              unsigned char** buffer) {
    *buffer = (unsigned char*)calloc(channels * width * height, sizeof(unsigned char));
    *surf = cairo_image_surface_create_for_data(*buffer, CAIRO_FORMAT_ARGB32, width, height,
                                                channels * width);
    return cairo_create(*surf);
}

void get_text_size(PangoLayout* layout, int* width, int* height) {
    pango_layout_get_size(layout, width, height);
    *width /= PANGO_SCALE;
    *height /= PANGO_SCALE;
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(const char* utf8_str) {
    FT_Face ft_face = hb_ft_font_get_face(pimpl->hb_font);
    hb_codepoint_t glyph_index = pimpl->getGlyphIndex(utf8_str);

    PangoGlyph pango_glyph_index = pimpl->getPangoGlyph(utf8_str);
    fprintf(stderr, "%d vs %d\n", glyph_index, pango_glyph_index);

    cairo_t* layout_context = create_layout_context();
    PangoLayout* layout = pango_cairo_create_layout(layout_context);
    pango_layout_set_text(layout, utf8_str, -1);

    PangoFontDescription* desc = pango_font_describe(pimpl->pango_font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_width, text_height, texture_id;
    cairo_surface_t* temp_surface;
    cairo_surface_t* surface;
    unsigned char* surface_data = nullptr;
    get_text_size(layout, &text_width, &text_height);
    cairo_t* render_context =
        create_cairo_context(text_width, text_height, 4, &surface, &surface_data);

    cairo_set_source_rgba(render_context, 1, 1, 1, 1);
    pango_cairo_show_layout(render_context, layout);

    std::vector<uint8_t> temp_buffer;

    size_t pixels = text_width * text_height * 4;
    temp_buffer.reserve(pixels);
    for (size_t i = 0; i < pixels; i += 4) {
        temp_buffer.push_back(surface_data[i + 2]);
        temp_buffer.push_back(surface_data[i + 1]);
        temp_buffer.push_back(surface_data[i]);
        // temp_buffer.push_back(surface_data[i + 3]);
    }

    return RasterizedGlyph{
        .colored = false,
        .left = 0,
        .top = 0,
        .width = text_width,
        .height = text_height,
        .advance = static_cast<float>(text_width),
        .buffer = temp_buffer,
    };

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
        .colored = colored,
        .left = ft_face->glyph->bitmap_left,
        .top = top,
        .width = static_cast<int32_t>(width),
        .height = static_cast<int32_t>(rows),
        .advance = advance,
        .buffer = packed_buffer,
    };
}

std::vector<RasterizedGlyph> FontRasterizer::layoutLine(const char* utf8_str) {
    return {};
}

FontRasterizer::~FontRasterizer() {
    hb_font_destroy(pimpl->hb_font);
}
