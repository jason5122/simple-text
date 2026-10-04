#include "fx/win/dwrite_globals.h"
#include <algorithm>
#include <cmath>
#include <vector>

using Microsoft::WRL::ComPtr;

const dwrite_globals& globals() {
    static const dwrite_globals g = [] {
        dwrite_globals result;
        if (SUCCEEDED(DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
                reinterpret_cast<IUnknown**>(result.factory2.GetAddressOf())))) {
            result.factory2.As(&result.factory);
        } else {
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                reinterpret_cast<IUnknown**>(result.factory.GetAddressOf()));
        }
        if (result.factory) {
            result.factory.As(&result.factory3);
            result.factory->GetGdiInterop(&result.gdi_interop);
            POINT origin{};
            HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
            result.factory->CreateMonitorRenderingParams(monitor, &result.rendering_params);
            if (!result.rendering_params) {
                result.factory->CreateRenderingParams(&result.rendering_params);
            }
        }
        return result;
    }();
    return g;
}

namespace {

// One IDWriteRenderingParams per monitor, indexed by the same `platform_value` that selects the
// glyph cache's table. Sublime keeps the equivalent array at 0x140709288. Append-only, so an index
// handed out once keeps pointing at the same display's parameters.
std::vector<ComPtr<IDWriteRenderingParams>>& monitor_params() {
    static std::vector<ComPtr<IDWriteRenderingParams>> params;
    return params;
}

struct monitor_scan {
    HMONITOR target = nullptr;
    uint32_t index = 0;
};

BOOL CALLBACK scan_monitors(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    monitor_scan& scan = *reinterpret_cast<monitor_scan*>(data);
    std::vector<ComPtr<IDWriteRenderingParams>>& params = monitor_params();
    if (params.size() <= scan.index) {
        params.resize(scan.index + 1);
    }
    if (!params[scan.index] &&
        FAILED(globals().factory->CreateMonitorRenderingParams(monitor, &params[scan.index]))) {
        // Sublime abandons the walk and falls back to slot zero rather than leaving a hole
        // (0x1401cb635).
        scan.index = 0;
        return FALSE;
    }
    if (monitor == scan.target) {
        return FALSE;
    }
    ++scan.index;
    return TRUE;
}

// The parameters for one display, falling back to the process-wide primary-monitor set whenever
// the index is one we have never enumerated.
ComPtr<IDWriteRenderingParams> monitor_params_for(uint32_t platform_value) {
    const std::vector<ComPtr<IDWriteRenderingParams>>& params = monitor_params();
    if (platform_value < params.size() && params[platform_value]) {
        return params[platform_value];
    }
    return globals().rendering_params;
}

}  // namespace

uint32_t fx_monitor_platform_value(void* monitor) {
    if (!monitor || !globals().factory) {
        return 0;
    }
    monitor_scan scan{.target = static_cast<HMONITOR>(monitor)};
    // EnumDisplayMonitors reports false exactly when a callback stopped it, which here means the
    // walk reached `target` (or gave up and reset the index). A complete walk means the monitor is
    // not among the displays, so fall back to the first slot, as Sublime does (0x1401bfc90).
    if (EnumDisplayMonitors(nullptr, nullptr, scan_monitors, reinterpret_cast<LPARAM>(&scan))) {
        return 0;
    }
    return scan.index;
}

// The cache applies the monitor correction after rasterization, as Sublime's gl_glyph_cache does.
// Keep the intermediate coverage mask neutral apart from DirectWrite's normalized contrast term;
// grayscale and aliased modes additionally zero ClearType and flatten the pixel geometry.
ComPtr<IDWriteRenderingParams> params_for(uint32_t flags, uint32_t platform_value) {
    ComPtr<IDWriteRenderingParams> base = monitor_params_for(platform_value);
    if (!base) return base;
    if (!(flags & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS))) {
        // The ordinary DirectWrite path hands the selected monitor parameters straight to
        // IDWriteBitmapRenderTarget::DrawGlyphRun (0x1401bbf2d).  A neutral intermediate mask
        // changes the coverage curve before the glyph-cache gamma table ever sees it.
        return base;
    }

    bool grayscale = (flags & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS)) != 0;
    DWRITE_RENDERING_MODE mode =
        (flags & FX_FONT_NO_ANTIALIAS) ? DWRITE_RENDERING_MODE_ALIASED : base->GetRenderingMode();
    FLOAT normalized_contrast = std::min(base->GetEnhancedContrast(), 4.0f) / 5.0f;
    FLOAT gamma = 1.0f + normalized_contrast;
    constexpr FLOAT contrast = 0.0f;
    FLOAT cleartype_level = base->GetClearTypeLevel();

    ComPtr<IDWriteRenderingParams> custom;
    if (FAILED(globals().factory->CreateCustomRenderingParams(
            gamma, contrast, grayscale ? 0.0f : cleartype_level,
            grayscale ? DWRITE_PIXEL_GEOMETRY_FLAT : base->GetPixelGeometry(), mode, &custom))) {
        return base;
    }
    return custom;
}

fx_gamma_ramp rendering_gamma_ramp() {
    fx_gamma_ramp ramp;
    ComPtr<IDWriteRenderingParams> params = globals().rendering_params;
    // Sublime folds DirectWrite's enhanced-contrast setting into the cache gamma
    // (0x1401bb8dd..0x1401bb91e). Both inputs are FLOAT, and the binary keeps the whole
    // expression in scalar SSE, so the exponent stays float here too.
    double exponent = 1.0;
    if (params) {
        float contrast = std::min(params->GetEnhancedContrast(), 4.0f) / 5.0f;
        exponent = std::max(params->GetGamma() - contrast, 1.0f) - contrast;
    }
    for (size_t i = 0; i < ramp.values.size(); ++i) {
        double input = static_cast<double>(i) / 255.0;
        ramp.values[i] = static_cast<uint8_t>(
            std::min(255.0, std::floor(255.0 * std::pow(input, exponent) + 0.5)));
    }
    return ramp;
}
