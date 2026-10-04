#include "fx/win/dwrite_raster_target.h"
#include "fx/win/dwrite_globals.h"
#include <cstdlib>
#include <limits>
#include <spdlog/spdlog.h>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

// fx clamps a pathological glyph to INT_MAX - 1 before calling rasterize(); doubling that would
// overflow the guard back into UB, so saturate and let CreateBitmapRenderTarget fail cleanly.
int doubled(int n) {
    return n < std::numeric_limits<int>::max() / 2 ? n * 2 : std::numeric_limits<int>::max();
}

}  // namespace

bool dwrite_raster_target::ensure(int width, int height, double scale) {
    const dwrite_globals& g = globals();
    if (!g.gdi_interop) return false;

    if (!target_ || width_ < width || height_ < height) {
        target_.Reset();
        target1_.Reset();
        width_ = doubled(width);
        height_ = doubled(height);
        if (FAILED(g.gdi_interop->CreateBitmapRenderTarget(
                nullptr, static_cast<UINT32>(width_), static_cast<UINT32>(height_), &target_))) {
            width_ = height_ = 0;
            return false;
        }
        target_.As(&target1_);
    }
    // Sublime only sets this when it creates the target, since one font is bound to one scale
    // there. rasterize() takes the scale per call, so keep the target in step with it.
    target_->SetPixelsPerDip(static_cast<FLOAT>(scale));
    return true;
}

namespace {

// Mean of the three coverage channels, already positioned as the destination alpha byte.
uint32_t mean_coverage(uint32_t bgra) {
    uint32_t blue = bgra & 0xFF;
    uint32_t green = (bgra >> 8) & 0xFF;
    uint32_t red = (bgra >> 16) & 0xFF;
    return ((blue + green + red) / 3) << 24;
}

}  // namespace

void dwrite_raster_target::read(int w, int h, bool keep_alpha, fx_pixel_buffer* out) const {
    DIBSECTION dib{};
    HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(target_->GetMemoryDC(), OBJ_BITMAP));
    if (GetObjectW(bitmap, sizeof(dib), &dib) == 0 || !dib.dsBm.bmBits) return;
    // CreateBitmapRenderTarget clamps an outsized request instead of failing, so the DIB can come
    // back smaller than ensure() asked for.
    if (dib.dsBm.bmBitsPixel != 32 || dib.dsBm.bmWidth < w || dib.dsBm.bmHeight < h) return;

    const auto* src = static_cast<const uint32_t*>(dib.dsBm.bmBits);
    size_t stride = static_cast<size_t>(dib.dsBm.bmWidthBytes) / 4;
    for (size_t y = 0; y < static_cast<size_t>(h); y++) {
        for (size_t x = 0; x < static_cast<size_t>(w); x++) {
            uint32_t pixel = src[y * stride + x];
            out->pixels[y * static_cast<size_t>(out->row_pixels) + x] =
                mean_coverage(pixel) | (keep_alpha ? pixel : pixel & 0x00FFFFFF);
        }
    }
}

void draw_color_layers(IDWriteBitmapRenderTarget* target,
                       IDWriteColorGlyphRunEnumerator* layers,
                       IDWriteRenderingParams* params,
                       double origin_x,
                       double origin_y) {
    static bool trace_color = std::getenv("PX_TRACE_COLOR") != nullptr;
    for (;;) {
        BOOL has_run = FALSE;
        if (FAILED(layers->MoveNext(&has_run)) || !has_run) break;
        const DWRITE_COLOR_GLYPH_RUN* layer = nullptr;
        if (FAILED(layers->GetCurrentRun(&layer))) break;
        if (trace_color && layer->runColor.a != 1.0f) {
            spdlog::info("color layer glyph {} rgba {:.6f} {:.6f} {:.6f} {:.6f}",
                         layer->glyphRun.glyphIndices[0], layer->runColor.r, layer->runColor.g,
                         layer->runColor.b, layer->runColor.a);
        }
        COLORREF color = RGB(static_cast<int>(layer->runColor.r * 255.0f),
                             static_cast<int>(layer->runColor.g * 255.0f),
                             static_cast<int>(layer->runColor.b * 255.0f));
        target->DrawGlyphRun(static_cast<FLOAT>(origin_x), static_cast<FLOAT>(origin_y),
                             DWRITE_MEASURING_MODE_NATURAL, &layer->glyphRun, params, color,
                             nullptr);
    }
}

bool rasterize_via_analysis(const DWRITE_GLYPH_RUN& run,
                            DWRITE_RENDERING_MODE rendering,
                            DWRITE_MEASURING_MODE measuring,
                            double scale,
                            double origin_x,
                            double origin_y,
                            int w,
                            int h,
                            fx_pixel_buffer* out) {
    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    if (FAILED(globals().factory->CreateGlyphRunAnalysis(
            &run, static_cast<FLOAT>(scale), nullptr, rendering, measuring,
            static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
            &analysis))) {
        return false;
    }

    RECT bounds = {0, 0, w, h};
    bool aliased = rendering == DWRITE_RENDERING_MODE_ALIASED;
    DWRITE_TEXTURE_TYPE type = aliased ? DWRITE_TEXTURE_ALIASED_1x1 : DWRITE_TEXTURE_CLEARTYPE_3x1;
    size_t samples = aliased ? 1 : 3;

    std::vector<BYTE> alpha(static_cast<size_t>(w) * static_cast<size_t>(h) * samples);
    if (FAILED(analysis->CreateAlphaTexture(type, &bounds, alpha.data(),
                                            static_cast<UINT32>(alpha.size())))) {
        return false;
    }

    for (size_t y = 0; y < static_cast<size_t>(h); ++y) {
        for (size_t x = 0; x < static_cast<size_t>(w); ++x) {
            size_t pixel = y * static_cast<size_t>(w) + x;
            uint32_t rgb = 0;
            for (size_t s = 0; s < 3; ++s) {
                uint32_t v = alpha[pixel * samples + (samples == 1 ? 0 : s)];
                rgb |= v << (8 * (2 - s));  // CLEARTYPE_3x1 is R, G, B in order
            }
            out->pixels[y * static_cast<size_t>(out->row_pixels) + x] = 0xFF000000 | rgb;
        }
    }
    return true;
}
