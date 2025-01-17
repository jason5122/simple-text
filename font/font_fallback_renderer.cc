#include "font_fallback_renderer.h"

#include "font/impl_directwrite.h"

using Microsoft::WRL::ComPtr;

// TODO: Debug use; remove this.
#include <fmt/base.h>

namespace font {

FontFallbackRenderer::FontFallbackRenderer(ComPtr<IDWriteFontCollection> font_collection,
                                           std::string_view str8)
    : ref_count(1), font_collection(font_collection) {
    if (!indices_map.set_utf8(str8.data(), str8.length())) {
        fmt::println("UTF16ToUTF8IndicesMap::setUTF8 error");
        std::abort();
    }
}

HRESULT FontFallbackRenderer::QueryInterface(IID const& riid, void** ppv_object) {
    if (__uuidof(IUnknown) == riid || __uuidof(IDWritePixelSnapping) == riid ||
        __uuidof(IDWriteTextRenderer) == riid) {
        *ppv_object = this;
        this->AddRef();
        return S_OK;
    }
    *ppv_object = nullptr;
    return E_FAIL;
}

ULONG FontFallbackRenderer::AddRef() {
    return InterlockedIncrement(&ref_count);
}

ULONG FontFallbackRenderer::Release() {
    ULONG new_count = InterlockedDecrement(&ref_count);
    if (new_count == 0) {
        delete this;
    }
    return new_count;
}

namespace {

// Invert cluster map (string index -> glyph index) to (glyph index -> string index).
std::vector<size_t> get_inverted_cluster_map(
    const DWRITE_GLYPH_RUN_DESCRIPTION* glyph_run_description, size_t glyph_count) {

    auto cluster_map = glyph_run_description->clusterMap;
    size_t len = glyph_run_description->stringLength;

    std::vector<size_t> inverted_cluster_map(glyph_count);
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && cluster_map[i] == cluster_map[i - 1]) {
            continue;
        }
        size_t glyph_index = cluster_map[i];
        inverted_cluster_map[glyph_index] = i;
    }
    return inverted_cluster_map;
}

}  // namespace

HRESULT FontFallbackRenderer::DrawGlyphRun(
    void* client_drawing_context,
    FLOAT baseline_origin_x,
    FLOAT baseline_origin_y,
    DWRITE_MEASURING_MODE measuring_mode,
    DWRITE_GLYPH_RUN const* glyph_run,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyph_run_description,
    IUnknown* client_drawing_effect) {

    if (!glyph_run->fontFace) {
        fmt::println("Glyph run without font face.");
        std::abort();
    }

    // Cache font.
    auto font_rasterizer = static_cast<FontRasterizer*>(client_drawing_context);
    ComPtr<IDWriteFont> font;
    font_collection->GetFontFromFontFace(glyph_run->fontFace, &font);
    int font_size = glyph_run->fontEmSize * 72 / 96;
    size_t run_font_id = font_rasterizer->cache_font({font}, font_size);

    size_t glyph_count = glyph_run->glyphCount;
    size_t text_position = glyph_run_description->textPosition;
    auto inverted_cluster_map = get_inverted_cluster_map(glyph_run_description, glyph_count);

    for (size_t i = 0; i < glyph_count; ++i) {
        uint32_t glyph_id = glyph_run->glyphIndices[i];
        // TODO: Verify that rounding up is correct.
        int advance = std::ceil(glyph_run->glyphAdvances[i]);

        size_t utf8_index = indices_map.map_index(text_position + inverted_cluster_map[i]);
        ShapedGlyph glyph = {
            .font_id = run_font_id,
            .glyph_id = glyph_id,
            // TODO: Do we need the y values?
            .position = {.x = total_advance},
            .advance = {.x = advance},
            .index = utf8_index,
        };
        glyphs.emplace_back(std::move(glyph));

        total_advance += advance;
    }

    return S_OK;
}

HRESULT FontFallbackRenderer::DrawUnderline(void* client_drawing_context,
                                            FLOAT baseline_origin_x,
                                            FLOAT baseline_origin_y,
                                            DWRITE_UNDERLINE const* underline,
                                            IUnknown* client_drawing_effect) {
    return E_NOTIMPL;
}

HRESULT FontFallbackRenderer::DrawStrikethrough(void* client_drawing_context,
                                                FLOAT baseline_origin_x,
                                                FLOAT baseline_origin_y,
                                                DWRITE_STRIKETHROUGH const* strikethrough,
                                                IUnknown* client_drawing_effect) {
    return E_NOTIMPL;
}

HRESULT FontFallbackRenderer::DrawInlineObject(void* client_drawing_context,
                                               FLOAT origin_x,
                                               FLOAT origin_y,
                                               IDWriteInlineObject* inline_object,
                                               BOOL is_sideways,
                                               BOOL is_right_to_left,
                                               IUnknown* client_drawing_effect) {
    return E_NOTIMPL;
}

HRESULT FontFallbackRenderer::IsPixelSnappingDisabled(void* client_drawing_context,
                                                      BOOL* is_disabled) {
    *is_disabled = false;
    return S_OK;
}

HRESULT FontFallbackRenderer::GetCurrentTransform(void* client_drawing_context,
                                                  DWRITE_MATRIX* transform) {
    static constexpr DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    *transform = ident;
    return S_OK;
}

HRESULT FontFallbackRenderer::GetPixelsPerDip(void* client_drawing_context,
                                              FLOAT* pixels_per_dip) {
    *pixels_per_dip = 1.0f;
    return S_OK;
}

}  // namespace font
