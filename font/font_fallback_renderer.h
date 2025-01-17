#pragma once

#include <string_view>
#include <vector>

#include <dwrite_3.h>
#include <wrl/client.h>

#include "font/types.h"
#include "unicode/utf16_to_utf8_indices_map.h"

namespace font {

class FontFallbackRenderer : public IDWriteTextRenderer {
public:
    FontFallbackRenderer(Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection,
                         std::string_view str8);

    // IUnknown methods.
    HRESULT WINAPI QueryInterface(IID const& riid, void** ppv_object) override;
    ULONG WINAPI AddRef() override;
    ULONG WINAPI Release() override;

    // IDWriteTextRenderer methods.
    HRESULT WINAPI DrawGlyphRun(void* client_drawing_context,
                                FLOAT baseline_origin_x,
                                FLOAT baseline_origin_y,
                                DWRITE_MEASURING_MODE measuring_mode,
                                DWRITE_GLYPH_RUN const* glyph_run,
                                DWRITE_GLYPH_RUN_DESCRIPTION const* glyph_run_description,
                                IUnknown* client_drawing_effect) override;
    HRESULT WINAPI DrawUnderline(void* client_drawing_context,
                                 FLOAT baseline_origin_x,
                                 FLOAT baseline_origin_y,
                                 DWRITE_UNDERLINE const* underline,
                                 IUnknown* client_drawing_effect) override;
    HRESULT WINAPI DrawStrikethrough(void* client_drawing_context,
                                     FLOAT baseline_origin_x,
                                     FLOAT baseline_origin_y,
                                     DWRITE_STRIKETHROUGH const* strikethrough,
                                     IUnknown* client_drawing_effect) override;
    HRESULT WINAPI DrawInlineObject(void* client_drawing_context,
                                    FLOAT origin_x,
                                    FLOAT origin_y,
                                    IDWriteInlineObject* inline_object,
                                    BOOL is_sideways,
                                    BOOL is_right_to_left,
                                    IUnknown* client_drawing_effect) override;

    // IDWritePixelSnapping methods.
    HRESULT WINAPI IsPixelSnappingDisabled(void* client_drawing_context,
                                           BOOL* is_disabled) override;
    HRESULT WINAPI GetCurrentTransform(void* client_drawing_context,
                                       DWRITE_MATRIX* transform) override;
    HRESULT WINAPI GetPixelsPerDip(void* client_drawing_context, FLOAT* pixels_per_dip) override;

private:
    virtual ~FontFallbackRenderer() {}

    ULONG ref_count;
    Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection;
    unicode::UTF16ToUTF8IndicesMap indices_map;

    // Outputs for FontRasterizer.
    friend class FontRasterizer;
    int total_advance = 0;
    std::vector<ShapedGlyph> glyphs;
};

}  // namespace font
