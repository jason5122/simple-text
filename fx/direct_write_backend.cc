#include "base/check.h"
#include "base/numeric/safe_conversions.h"
#include "base/strings.h"
#include "base/unicode.h"
#include "fx/fx.h"
#include "fx/fx_internal.h"
#include "fx/win/dwrite_globals.h"
#include "fx/win/dwrite_shaping.h"
#include "fx/win/dwrite_raster_target.h"
#include <windows.h>
// clang-format off: windows.h supplies the GDI types dwrite_3.h uses in its bitmap render target.
#include <dwrite_3.h>
#include <wrl/client.h>
// clang-format on
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

// DirectWrite backend, written to mirror Sublime Text's. Behavioural notes below cite addresses in
// the Windows build disassembled under conformance/ (sublime_text.exe, build 4200 x64).
//
// The shape is: Sublime never touches IDWriteTextAnalyzer or IDWriteFontFallback. It hands a
// string to IDWriteTextLayout, implements IDWriteTextRenderer, and scrapes the glyph runs that
// IDWriteTextLayout::Draw calls back with -- so DirectWrite performs both shaping and font
// fallback, and Sublime only records which face each run resolved to.

using Microsoft::WRL::ComPtr;

namespace {

// DirectWrite-specific bits in Sublime's font_options field, decoded from the settings parser at
// 0x14013b438. Cross-platform style, rasterization, and OpenType feature bits live in fx.h.
enum DirectWriteFlags : uint32_t {
    kDirectWrite = 1u << 5,
    kGdi = 1u << 6,
    kClearTypeClassic = 1u << 9,   // "dwrite_cleartype_classic"
    kClearTypeNatural = 1u << 10,  // "dwrite_cleartype_natural"
    kGdiCompatible = kClearTypeClassic | kClearTypeNatural,
};

// Sublime's default: ClearType, which puts a different coverage value in each of R/G/B. The
// renderer consumes all three via dual-source blending, so no flag is needed here. Setting
// FX_FONT_GRAY_ANTIALIAS collapses it to a single coverage, which is what a renderer without
// dual-source blending would need.
constexpr uint32_t kDefaultFlags = 0;

// Sublime rounds with floor(x + 0.4999999999999998) rather than std::round (0x1401bba8f). The
// nudged constant avoids the double-rounding std::round can hit, and it breaks ties downward where
// std::round breaks them away from zero -- which changes the pixel a baseline lands on.
double st_round(double x) {
    constexpr double kHalf = 0.4999999999999998;
    return x < 0 ? std::ceil(x - kHalf) : std::floor(x + kHalf);
}

bool starts_with_ci(std::string_view s, std::string_view prefix) {
    if (s.size() < prefix.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), s.begin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

class direct_write_font final : public fx_font {
public:
    static std::unique_ptr<direct_write_font> create(std::string family,
                                                     float size,
                                                     uint32_t attrs);
    static std::unique_ptr<direct_write_font> create_from_file(std::string path,
                                                               float size,
                                                               uint32_t attrs);

    uint32_t attrs() const override { return attrs_; }
    fx_font_metrics metrics() const override;
    float raster_ascent() const override { return raster_ascent_; }
    std::unique_ptr<fx_layout> shape(std::string_view utf8) override;
    void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) override;
    void rasterize(uint32_t glyph,
                   vec2 position,
                   float scale,
                   fx_pixel_buffer* buffer,
                   color foreground,
                   uint32_t platform_value) override;
    bool is_color_glyph(uint32_t glyph) override;
    const fx_gamma_ramp* gamma_ramp() const override { return &gamma_; }

private:
    explicit direct_write_font(uint32_t attrs)
        : flags_(attrs), attrs_(attrs), gamma_(rendering_gamma_ramp()) {}

    // Builds the font around a resolved IDWriteFont. `collection` is what the text format shapes
    // from: null for the system collection, or the private one wrapping a font file.
    static std::unique_ptr<direct_write_font> finish(IDWriteFont* font,
                                                     IDWriteFontCollection* collection,
                                                     float size_px,
                                                     uint32_t attrs);

    uint32_t register_face(IDWriteFontFace* face);

    ComPtr<IDWriteTextFormat> format;
    // ST stores these as parallel vectors at direct_write_font+0x48 and +0x60. Index 0 is the
    // primary face; shaping appends fallbacks and their unrounded ascents together.
    std::vector<ComPtr<IDWriteFontFace>> faces;
    std::vector<float> face_ascents;
    std::string family;
    float em_size_ = 0;
    uint32_t flags_ = kDefaultFlags;

    // Sublime's direct_write_font+0x3c/0x40/0x44. lineGap lives in the ascent, and line_height is
    // a separate rounding of the whole sum rather than ascent + descent.
    float ascent_ = 0;
    float raster_ascent_ = 0;
    float descent_ = 0;
    float line_height_ = 0;

    // Sublime caches one bitmap render target per font (+0x28/+0x30) and grows it on demand.
    dwrite_raster_target raster_;

    uint32_t attrs_ = 0;
    fx_gamma_ramp gamma_;
};

// Appends `face` to the font's registry if it isn't already there and returns its index. Sublime
// does this linear scan inside DrawGlyphRun (0x1401bc7a7); the registry is a member of the font,
// not of one shaping call, so indices stay valid for the font's lifetime.
uint32_t direct_write_font::register_face(IDWriteFontFace* face) {
    for (size_t i = 0; i < faces.size(); i++) {
        if (faces[i].Get() == face) return base::checked_cast<uint32_t>(i);
    }

    DWRITE_FONT_METRICS metrics{};
    if (flags_ & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(em_size_, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    float upem_scale =
        metrics.designUnitsPerEm ? em_size_ / static_cast<float>(metrics.designUnitsPerEm) : 0;

    faces.emplace_back(face);
    face_ascents.push_back(static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale);
    return base::checked_cast<uint32_t>(faces.size() - 1);
}

// ST's RTTI names this run_visitor<direct_write_font::shape(...)::<lambda_0>>. The visitor owns
// only the COM plumbing and forwards each glyph run to the shaping lambda stored by value.
template <typename Callback>
class run_visitor final : public IDWriteTextRenderer {
public:
    explicit run_visitor(Callback callback) : callback_(std::move(callback)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) ||
            riid == __uuidof(IDWriteTextRenderer)) {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_FAIL;  // Sublime returns E_FAIL here, not E_NOINTERFACE (0x1401bc712).
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 0; }
    ULONG STDMETHODCALLTYPE Release() override { return 0; }

    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* disabled) override {
        *disabled = FALSE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* transform) override {
        *transform = {1, 0, 0, 1, 0, 0};
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* pixels_per_dip) override {
        *pixels_per_dip = 1.0f;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*,
                                           FLOAT baseline_origin_x,
                                           FLOAT baseline_origin_y,
                                           DWRITE_MEASURING_MODE,
                                           const DWRITE_GLYPH_RUN* run,
                                           const DWRITE_GLYPH_RUN_DESCRIPTION* desc,
                                           IUnknown*) override {
        callback_(baseline_origin_x, baseline_origin_y, run, desc);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE
    DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE
    DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE
    DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) override {
        return S_OK;
    }

private:
    Callback callback_;
};

// Sublime's direct_write_font::glyph_extents (0x1401bb4f8).
DWRITE_RENDERING_MODE rendering_mode(uint32_t flags) {
    if (flags & FX_FONT_NO_ANTIALIAS) return DWRITE_RENDERING_MODE_ALIASED;
    if (flags & kClearTypeClassic) return DWRITE_RENDERING_MODE_GDI_CLASSIC;
    if (flags & kClearTypeNatural) return DWRITE_RENDERING_MODE_GDI_NATURAL;
    return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
}

DWRITE_MEASURING_MODE measuring_mode(uint32_t flags) {
    if (flags & kClearTypeClassic) return DWRITE_MEASURING_MODE_GDI_CLASSIC;
    if (flags & kClearTypeNatural) return DWRITE_MEASURING_MODE_GDI_NATURAL;
    return DWRITE_MEASURING_MODE_NATURAL;
}

std::unique_ptr<direct_write_font> direct_write_font::create(std::string family,
                                                             float size_px,
                                                             uint32_t attrs) {
    const dwrite_globals& g = globals();
    if (!g.factory || !g.gdi_interop) {
        spdlog::error("DirectWrite is unavailable");
        return nullptr;
    }

    // Sublime resolves the family through GDI rather than IDWriteFontCollection::FindFamilyName:
    // it fills a LOGFONTW and calls CreateFontFromLOGFONT (0x1401bd51a, 0x1401bc214), which gets
    // GDI's family aliasing and substitution for free. lfHeight is the negated point size
    // truncated to an integer, and only selects the physical font -- the size DirectWrite renders
    // at comes from CreateTextFormat in finish().
    // Sublime resolves its own "system" alias to Segoe UI before it ever reaches the font layer
    // (0x1401cebed).
    if (family == "system") family = "Segoe UI";

    LOGFONTW logfont{};
    logfont.lfHeight = -base::clamp_floor<LONG>(size_px);
    logfont.lfWeight = (attrs & FX_FONT_BOLD) ? FW_BOLD : 0;
    logfont.lfItalic = (attrs & FX_FONT_ITALIC) ? TRUE : FALSE;
    logfont.lfQuality = DEFAULT_QUALITY;
    if (attrs & FX_FONT_SUBPIXEL_ANTIALIAS) {
        logfont.lfQuality = CLEARTYPE_QUALITY;
    } else if (attrs & FX_FONT_GRAY_ANTIALIAS) {
        logfont.lfQuality = ANTIALIASED_QUALITY;
    } else if (attrs & FX_FONT_NO_ANTIALIAS) {
        logfont.lfQuality = NONANTIALIASED_QUALITY;
    }
    auto family16 = base::utf8_to_utf16(family);
    std::copy_n(family16.begin(), std::min<size_t>(family16.size(), LF_FACESIZE - 1),
                logfont.lfFaceName);

    ComPtr<IDWriteFont> font;
    if (FAILED(g.gdi_interop->CreateFontFromLOGFONT(&logfont, &font))) {
        spdlog::error("could not resolve font family \"{}\"", family);
        return nullptr;
    }

    std::unique_ptr<direct_write_font> result = finish(font.Get(), nullptr, size_px, attrs);
    if (!result) return nullptr;

    // CreateFontFromLOGFONT substitutes rather than failing, so an unavailable family comes back
    // as a different one with no error. Sublime reports the same substitution ("font face ...
    // could not be found, defaulting to ...") instead of rendering the wrong face quietly.
    if (!starts_with_ci(result->family, family)) {
        spdlog::warn("font face \"{}\" could not be found, defaulting to \"{}\"", family,
                     result->family);
    }
    return result;
}

std::unique_ptr<direct_write_font> direct_write_font::create_from_file(std::string path,
                                                                       float size_px,
                                                                       uint32_t attrs) {
    const dwrite_globals& g = globals();
    if (!g.factory) {
        spdlog::error("DirectWrite is unavailable");
        return nullptr;
    }
    if (!g.factory3) {
        spdlog::error("loading a font file requires DirectWrite 3 (Windows 10)");
        return nullptr;
    }

    // A font file is in neither the system collection nor GDI's tables, so neither lookup in
    // create() can see it. Wrap it in a one-font collection instead: IDWriteTextLayout then
    // shapes with it exactly as it would with a system family, including fallback to system
    // fonts for characters the file lacks.
    auto path16 = base::utf8_to_utf16(path);
    ComPtr<IDWriteFontFile> file;
    if (FAILED(g.factory->CreateFontFileReference(base::as_wcstr(path16), nullptr, &file))) {
        spdlog::error("could not open font file \"{}\"", path);
        return nullptr;
    }

    // Parse the file before it goes anywhere near a font set. IDWriteFontSetBuilder accepts a
    // reference to anything -- AddFontFaceReference returns S_OK for an empty file -- and then
    // CreateFontSet crashes with an access violation when the face behind it cannot be created,
    // seen on Windows 11 arm64 with an empty file, a text file, a truncated font, and a font whose
    // unitsPerEm is zero. IDWriteFactory::CreateFontFace is the path that reports those as
    // DWRITE_E_FILEFORMAT instead.
    BOOL supported = FALSE;
    DWRITE_FONT_FILE_TYPE file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
    DWRITE_FONT_FACE_TYPE face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
    UINT32 face_count = 0;
    ComPtr<IDWriteFontFace> probe;
    IDWriteFontFile* files[] = {file.Get()};
    DWRITE_FONT_METRICS probe_metrics{};
    if (FAILED(file->Analyze(&supported, &file_type, &face_type, &face_count)) || !supported ||
        face_count == 0 ||
        FAILED(g.factory->CreateFontFace(face_type, 1, files, 0, DWRITE_FONT_SIMULATIONS_NONE,
                                         &probe))) {
        spdlog::error("\"{}\" is not a usable font file", path);
        return nullptr;
    }
    // finish() refuses a zero em for the metrics it derives; the set builder has to be spared it
    // as well.
    probe->GetMetrics(&probe_metrics);
    if (probe_metrics.designUnitsPerEm == 0) {
        spdlog::error("\"{}\" has zero units per em", path);
        return nullptr;
    }

    ComPtr<IDWriteFontFaceReference> reference;
    ComPtr<IDWriteFontSetBuilder> builder;
    ComPtr<IDWriteFontSet> set;
    ComPtr<IDWriteFontCollection1> collection;
    if (FAILED(g.factory3->CreateFontFaceReference(file.Get(), 0, DWRITE_FONT_SIMULATIONS_NONE,
                                                   &reference)) ||
        FAILED(g.factory3->CreateFontSetBuilder(&builder)) ||
        FAILED(builder->AddFontFaceReference(reference.Get())) ||
        FAILED(builder->CreateFontSet(&set)) ||
        FAILED(g.factory3->CreateFontCollectionFromFontSet(set.Get(), &collection)) ||
        collection->GetFontFamilyCount() == 0) {
        spdlog::error("\"{}\" is not a usable font file", path);
        return nullptr;
    }
    ComPtr<IDWriteFontFamily> family;
    ComPtr<IDWriteFont> font;
    if (FAILED(collection->GetFontFamily(0, &family)) || FAILED(family->GetFont(0, &font))) {
        return nullptr;
    }
    return finish(font.Get(), collection.Get(), size_px, attrs);
}

std::unique_ptr<direct_write_font> direct_write_font::finish(IDWriteFont* font,
                                                             IDWriteFontCollection* collection,
                                                             float size_px,
                                                             uint32_t attrs) {
    auto result = std::unique_ptr<direct_write_font>(new direct_write_font(attrs));
    direct_write_font& data = *result;
    data.em_size_ = size_px;

    ComPtr<IDWriteFontFamily> font_family;
    ComPtr<IDWriteLocalizedStrings> family_names;
    if (FAILED(font->GetFontFamily(&font_family)) ||
        FAILED(font_family->GetFamilyNames(&family_names))) {
        return nullptr;
    }
    UINT32 name_length = 0;
    family_names->GetStringLength(0, &name_length);
    std::u16string resolved(name_length + 1, u'\0');
    family_names->GetString(0, base::as_writable_wcstr(resolved), name_length + 1);
    resolved.resize(name_length);
    data.family = base::utf16_to_utf8(resolved);

    if (FAILED(globals().factory->CreateTextFormat(
            base::as_wcstr(resolved), collection, font->GetWeight(), font->GetStyle(),
            font->GetStretch(), data.em_size_, L"en-us", &data.format))) {
        return nullptr;
    }

    DWRITE_FONT_METRICS metrics{};
    font->GetMetrics(&metrics);
    // register_face() and extents() both refuse a zero em; without the same guard here a
    // malformed file would leave every metric NaN and propagate that into fx_layout.
    if (metrics.designUnitsPerEm == 0) return nullptr;
    float upem_scale = data.em_size_ / static_cast<float>(metrics.designUnitsPerEm);
    data.raster_ascent_ = static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale;
    data.ascent_ = std::ceil(data.raster_ascent_);
    data.descent_ = std::ceil(static_cast<float>(metrics.descent) * upem_scale);
    data.line_height_ = std::round(
        static_cast<float>(metrics.ascent + metrics.descent + metrics.lineGap) * upem_scale);
    // A hardcoded Segoe fudge in Sublime, presumably to match some other measurement of the
    // Windows UI font.
    if (starts_with_ci(data.family, "segoe")) {
        data.raster_ascent_ -= 1.0f;
        data.ascent_ -= 1.0f;
        data.line_height_ -= 1.0f;
    }

    // Sublime lets the registry fill itself from the first glyph run. Seeding index 0 here instead
    // keeps a freshly created font usable before anything has been shaped; DirectWrite resolves
    // the primary run to this same face, so the index is unchanged.
    ComPtr<IDWriteFontFace> face;
    if (FAILED(font->CreateFontFace(&face))) return nullptr;
    data.register_face(face.Get());
    return result;
}

}  // namespace

void assign_clusters(const DWRITE_GLYPH_RUN_DESCRIPTION* desc,
                     const std::vector<size_t>& indices_map,
                     std::vector<fx_glyph>& glyphs) {
    if (!desc || !desc->clusterMap || !desc->string) return;

    constexpr uint32_t kUnset = UINT32_MAX;
    for (auto& glyph : glyphs) glyph.cluster = kUnset;

    size_t previous = kUnset;
    for (UINT32 i = 0; i < desc->stringLength; i++) {
        UINT32 text_position = desc->textPosition + i;
        wchar_t unit = desc->string[i];
        if ((unit & 0xFC00) == 0xDC00) continue;

        size_t glyph = desc->clusterMap[i];
        if (glyph >= glyphs.size() || glyph == previous) continue;
        if (text_position >= indices_map.size()) break;
        glyphs[glyph].cluster = base::checked_cast<uint32_t>(indices_map[text_position]);
        previous = glyph;
    }

    uint32_t last = 0;
    for (auto& glyph : glyphs) {
        if (glyph.cluster == kUnset) glyph.cluster = last;
        else last = glyph.cluster;
    }
}

namespace {

std::unique_ptr<fx_layout> direct_write_font::shape(std::string_view utf8) {
    DCHECK(base::is_valid_utf8(utf8));

    auto shaped = std::make_unique<fx_layout>();
    shaped->line_height = line_height_;

    auto utf16 = base::utf8_to_utf16(utf8);
    UINT32 length = base::checked_cast<UINT32>(utf16.size());
    if (length == 0) return shaped;

    constexpr FLOAT kUnbounded = std::numeric_limits<FLOAT>::max();
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr;
    if (flags_ & kGdiCompatible) {
        hr = globals().factory->CreateGdiCompatibleTextLayout(
            base::as_wcstr(utf16), length, format.Get(), kUnbounded, kUnbounded, 1.0f, nullptr,
            (flags_ & kClearTypeNatural) ? TRUE : FALSE, &layout);
    } else {
        hr = globals().factory->CreateTextLayout(base::as_wcstr(utf16), length, format.Get(),
                                                 kUnbounded, kUnbounded, &layout);
    }
    if (FAILED(hr)) return nullptr;

    // Sublime always attaches a typography object, and always names liga/clig/calt explicitly --
    // enabled unless the matching font_option turned them off (0x1401bb105). Note that
    // SetTypography replaces DirectWrite's per-script defaults rather than adding to them, so this
    // list is the complete feature set Sublime renders with.
    ComPtr<IDWriteTypography> typography;
    if (SUCCEEDED(globals().factory->CreateTypography(&typography)) && typography) {
        auto feature = [&](DWRITE_FONT_FEATURE_TAG tag, bool on) {
            typography->AddFontFeature({tag, on ? 1u : 0u});
        };
        feature(DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, !(flags_ & FX_FONT_NO_LIGA));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, !(flags_ & FX_FONT_NO_CLIG));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, !(flags_ & FX_FONT_NO_CALT));
        if (flags_ & FX_FONT_DLIG) {
            feature(DWRITE_FONT_FEATURE_TAG_DISCRETIONARY_LIGATURES, true);
        }
        // ss01..ss09 differ only in their last character, but ss10 changes two of them, so the
        // tags are spelled out rather than derived by arithmetic.
        static constexpr DWRITE_FONT_FEATURE_TAG kStylisticSets[] = {
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_1, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_2,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_3, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_4,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_5, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_6,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_8,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_9, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_10,
        };
        for (uint32_t i = 0; i < std::size(kStylisticSets); i++) {
            if (flags_ & (FX_FONT_SS01 << i)) feature(kStylisticSets[i], true);
        }
        layout->SetTypography(typography.Get(), {0, length});
    }

    std::vector<size_t> indices_map = base::utf16_to_utf8_offsets(utf8);
    auto visit_run = [this, &indices_map, &shaped](
                         FLOAT baseline_origin_x, FLOAT baseline_origin_y,
                         const DWRITE_GLYPH_RUN* run, const DWRITE_GLYPH_RUN_DESCRIPTION* desc) {
        if (!run->fontFace || run->glyphCount == 0 || !run->glyphIndices) return;

        uint32_t face = register_face(run->fontFace);
        size_t glyph_count = run->glyphCount;
        std::vector<fx_glyph> glyphs(glyph_count);
        float pen = 0.0f;
        for (size_t i = 0; i < glyph_count; i++) {
            // DirectWrite calls back once per run, so add the run's baseline x to each relative
            // glyph position. Convert its line-top-relative baseline to the baseline-relative
            // convention shared by the fx backends.
            float x = baseline_origin_x + pen;
            float y = std::round(face_ascents[0]) - baseline_origin_y;
            if (run->glyphOffsets) {
                x += run->glyphOffsets[i].advanceOffset;
                y -= run->glyphOffsets[i].ascenderOffset;
            }
            float advance = run->glyphAdvances ? run->glyphAdvances[i] : 0.0f;
            glyphs[i] = {
                .id = (face << 16) | static_cast<uint32_t>(run->glyphIndices[i]),
                .x_offset = x,
                .y_offset = y,
                .cluster = 0,
            };
            pen += advance;
        }

        assign_clusters(desc, indices_map, glyphs);

        shaped->advance += pen;
        shaped->glyphs.insert(shaped->glyphs.end(), glyphs.begin(), glyphs.end());
    };
    run_visitor visitor(std::move(visit_run));
    layout->Draw(nullptr, &visitor, 0.0f, 0.0f);
    return shaped;
}

void direct_write_font::rasterize(uint32_t glyph,
                                  vec2 position,
                                  float scale,
                                  fx_pixel_buffer* buffer,
                                  color foreground,
                                  uint32_t platform_value) {
    DCHECK(buffer);
    DCHECK(buffer->pixels);
    DCHECK(buffer->width > 0);
    DCHECK(buffer->height > 0);
    DCHECK(buffer->row_pixels >= buffer->width);
    uint32_t face_index = glyph >> 16;

    if (face_index >= faces.size() || scale <= 0.0f) {
        return;
    }
    int width = buffer->width;
    int height = buffer->height;

    UINT16 index = static_cast<uint16_t>(glyph);
    FLOAT advance = 0;
    DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN run = {
        .fontFace = faces[face_index].Get(),
        .fontEmSize = em_size_,
        .glyphCount = 1,
        .glyphIndices = &index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = FALSE,
        .bidiLevel = 0,
    };

    double origin_x = position.x;
    // The two roundings are deliberately different functions. The first reproduces Sublime's own
    // snapping of the drawn face's ascent (st_round, floor(x + 0.4999999999999998)); the second
    // must undo, bit for bit, the primary-face ascent that extents() folded into the origin with
    // std::round. Using one function for both shifts glyphs by a pixel at the sizes where the two
    // tie-break differently.
    double origin_y = position.y +
                      st_round(static_cast<double>(face_ascents[face_index]) * scale) -
                      std::round(static_cast<double>(face_ascents[0]) * scale);

    ComPtr<IDWriteColorGlyphRunEnumerator> color_layers;
    if (globals().factory2) {
        // Anything other than S_OK (DWRITE_E_NOCOLOR for the common case) means "not a color
        // glyph"; the out pointer is not meaningful then.
        if (FAILED(globals().factory2->TranslateColorGlyphRun(0.0f, 0.0f, &run, nullptr,
                                                              DWRITE_MEASURING_MODE_NATURAL,
                                                              nullptr, 0, &color_layers))) {
            color_layers.Reset();
        }
    }
    bool colored = color_layers != nullptr;

    // Sublime's primary path is IDWriteBitmapRenderTarget::DrawGlyphRun into a GDI DIB; fall back
    // to CreateGlyphRunAnalysis if the bitmap render target cannot be created (0x1401bb9c1).
    if (raster_.ensure(width, height, scale)) {
        HDC hdc = raster_.get()->GetMemoryDC();
        RECT rect = {0, 0, width, height};
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        if (raster_.get1()) {
            // Sublime sets this once when it creates the target and again before a color run,
            // which leaves the mode sticky afterwards; setting it per draw keeps mono glyphs
            // rendering the way the font options asked for.
            bool grayscale = colored || (flags_ & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS));
            raster_.get1()->SetTextAntialiasMode(grayscale ? DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE
                                                           : DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }

        ComPtr<IDWriteRenderingParams> params = params_for(flags_, platform_value);
        if (colored) {
            draw_color_layers(raster_.get(), color_layers.Get(), params.Get(), origin_x / scale,
                              origin_y / scale);
            raster_.read(width, height, /*keep_alpha=*/true, buffer);
        } else {
            raster_.get()->DrawGlyphRun(
                static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
                measuring_mode(flags_), &run, params.Get(),
                RGB(foreground.red(), foreground.green(), foreground.blue()), nullptr);
            raster_.read(width, height, /*keep_alpha=*/false, buffer);
        }
    } else if (!colored) {
        rasterize_via_analysis(run, rendering_mode(flags_), measuring_mode(flags_), scale, origin_x,
                               origin_y, width, height, buffer);
    }
}

fx_font_metrics direct_write_font::metrics() const {
    return {
        .ascent = ascent_,
        .descent = descent_,
        .leading = line_height_ - ascent_ - descent_,
        .line_height = line_height_,
    };
}

void direct_write_font::extents(uint32_t glyph, float scale, vec2& origin, vec2& size) {
    uint32_t face_index = glyph >> 16;
    if (face_index >= faces.size()) {
        origin = {};
        size = {};
        return;
    }
    IDWriteFontFace* face = faces[face_index].Get();

    DWRITE_FONT_METRICS metrics{};
    if (flags_ & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(em_size_, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    if (metrics.designUnitsPerEm == 0) {
        origin = {};
        size = {};
        return;
    }
    double upem_scale =
        static_cast<double>(em_size_ / static_cast<float>(metrics.designUnitsPerEm));

    UINT16 index = static_cast<uint16_t>(glyph);
    DWRITE_GLYPH_METRICS gm{};
    if (FAILED(face->GetDesignGlyphMetrics(&index, 1, &gm, FALSE))) {
        origin = {};
        size = {};
        return;
    }

    INT32 ink_w = static_cast<INT32>(gm.advanceWidth) - (gm.leftSideBearing + gm.rightSideBearing);
    INT32 ink_h =
        static_cast<INT32>(gm.advanceHeight) - (gm.topSideBearing + gm.bottomSideBearing);
    // A blank glyph, e.g. a space.
    if (ink_w <= 0 || ink_h <= 0) {
        origin = {};
        size = {};
        return;
    }

    // Sublime computes all of this inline here (0x1401bb4f8). It doubles the ink height before
    // scaling -- slack for a rasterizer that can paint outside the design box through hinting and
    // ClearType filtering, without having to measure first -- then pads the reported size by 2 on
    // both axes and the origin by 1. Both constants are literal doubles in its rdata.
    constexpr double kSizePad = 2.0;
    constexpr double kOriginPad = 1.0;
    size = {
        std::ceil(ink_w * upem_scale * scale) + kSizePad,
        std::ceil(ink_h * 2.0 * upem_scale * scale) + kSizePad,
    };

    double ink_top = static_cast<double>(gm.verticalOriginY - gm.topSideBearing) * upem_scale;
    double ascent = static_cast<double>(face_ascents[face_index]);
    origin = {
        std::ceil(static_cast<double>(-gm.leftSideBearing) * upem_scale * scale) + kOriginPad,
        // Sublime stops at the line above and lets render_glyph add the face ascent
        // (0x1401bba7b). We fold the primary face's snapped ascent in instead, so the shared glyph
        // cache measures bearings from a baseline-relative origin as it does on the other
        // backends; rasterize() subtracts it again before drawing.
        std::ceil((ink_top - ascent) * scale) + kOriginPad +
            std::round(static_cast<double>(face_ascents[0]) * scale),
    };
}

bool direct_write_font::is_color_glyph(uint32_t glyph) {
    uint32_t face_index = glyph >> 16;
    if (!globals().factory2 || face_index >= faces.size()) {
        return false;
    }

    UINT16 index = static_cast<uint16_t>(glyph);
    FLOAT advance = 0;
    DWRITE_GLYPH_OFFSET offset{};
    DWRITE_GLYPH_RUN run = {
        .fontFace = faces[face_index].Get(),
        .fontEmSize = em_size_,
        .glyphCount = 1,
        .glyphIndices = &index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = FALSE,
        .bidiLevel = 0,
    };
    ComPtr<IDWriteColorGlyphRunEnumerator> color_layers;
    HRESULT result = globals().factory2->TranslateColorGlyphRun(
        0.0f, 0.0f, &run, nullptr, DWRITE_MEASURING_MODE_NATURAL, nullptr, 0, &color_layers);
    return result == S_OK && color_layers != nullptr;
}

}  // namespace

std::unique_ptr<fx_font> fx_backend_create_font(std::string_view family,
                                                float size,
                                                uint32_t attrs) {
    return direct_write_font::create(std::string(family), size, attrs);
}

std::unique_ptr<fx_font> fx_backend_create_font_from_file(std::string_view path,
                                                          float size,
                                                          uint32_t attrs) {
    return direct_write_font::create_from_file(std::string(path), size, attrs);
}
