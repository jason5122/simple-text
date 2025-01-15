#include "font/font_rasterizer.h"

#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/apple/string_conversions.h"
#include "base/numeric/saturation_arithmetic.h"
#include "unicode/utf16_to_utf8_indices_map.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

// TODO: Debug use; remove this.
#include <cassert>
#include <fmt/base.h>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

// References:
// https://github.com/servo/font-kit/blob/d49041ca57da4e9b412951f96f74cb34e3b6324f/src/loader.rs#L199
// https://github.com/zed-industries/zed/blob/40ecc38dd25ffdec4deb6e27ee91b72e85a019eb/crates/gpui/src/platform/mac/text_system.rs#L345

namespace font {

namespace {
ScopedCFTypeRef<CTLineRef> CreateCTLine(CTFontRef ct_font, FontId font_id, std::string_view str8);
bool FontSmoothingEnabled();
}  // namespace

struct FontRasterizer::NativeFontType {
    ScopedCFTypeRef<CTFontRef> font;
};

class FontRasterizer::impl {};

FontRasterizer::FontRasterizer() {}

FontRasterizer::~FontRasterizer() {}

FontId FontRasterizer::add_font(std::string_view font_name8, int font_size, FontStyle style) {
    std::string font_style;
    if (style == FontStyle::kNone) {
        font_style = "Regular";
    } else if (style == FontStyle::kBold) {
        font_style = "Bold";
    } else if (style == FontStyle::kItalic) {
        font_style = "Italic";
    } else if (style == (FontStyle::kBold | FontStyle::kItalic)) {
        font_style = "Bold Italic";
    }

    auto font_name_cfstring = base::apple::StringToCFString(font_name8);
    auto font_style_cfstring = base::apple::StringToCFString(font_style);
    CFTypeRef keys[] = {kCTFontFamilyNameAttribute, kCTFontStyleNameAttribute};
    CFTypeRef values[] = {font_name_cfstring.get(), font_style_cfstring.get()};
    static_assert(std::size(keys) == std::size(values));
    auto attributes = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, values, std::size(keys),
                           &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    auto descriptor = ScopedCFTypeRef<CTFontDescriptorRef>(
        CTFontDescriptorCreateWithAttributes(attributes.get()));

    CTFontRef ct_font = CTFontCreateWithFontDescriptor(descriptor.get(), font_size, nullptr);
    return cache_font({ct_font}, font_size);
}

FontId FontRasterizer::add_system_font(int font_size, FontStyle style) {
    bool is_bold = (style & FontStyle::kBold) != FontStyle::kNone;
    CTFontUIFontType font_type = is_bold ? kCTFontUIFontEmphasizedSystem : kCTFontUIFontSystem;

    CTFontRef sys_font = CTFontCreateUIFontForLanguage(font_type, font_size, nullptr);
    return cache_font({sys_font}, font_size);
}

FontId FontRasterizer::resize_font(FontId font_id, int font_size) {
    const auto& font_ref = font_id_to_native[font_id].font;
    CTFontRef copy = CTFontCreateCopyWithAttributes(font_ref.get(), font_size, nullptr, nullptr);
    return cache_font({copy}, font_size);
}

RasterizedGlyph FontRasterizer::rasterize(FontId font_id, uint32_t glyph_id) const {
    CTFontRef font_ref = font_id_to_native[font_id].font.get();

    if (!font_ref) {
        fmt::println("FontRasterizer::rasterize() error: CTFontRef is null!");
        std::abort();
    }

    CGGlyph glyph_index = glyph_id;
    CGRect bounds = CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault,
                                                    &glyph_index, nullptr, 1);

    // TODO: Don't hard-code scale factor.
    int scale_factor = 2;

    int left = std::floor(bounds.origin.x);
    int descent = std::ceil(-bounds.origin.y);
    int ascent = std::ceil(bounds.origin.y + bounds.size.height);
    int width = std::ceil(bounds.origin.x + bounds.size.width);
    int height = ascent + descent;
    int top = ascent;

    width *= 2;
    height *= 2;
    top *= 2;
    // TODO: Why do we not scale `top` and `left` here?
    // left *= 2;
    // descent *= 2;

    if (width < 0 || height < 0) {
        fmt::println("Warning: width/height < 0 for font ID = {}, glyph ID = {}, font name = {}",
                     font_id, glyph_id, font_id_to_postscript_name[font_id]);
        return {};
    }

    std::vector<uint8_t> bitmap_data(height * width * 4);
    auto color_space_ref = ScopedTypeRef<CGColorSpaceRef>(CGColorSpaceCreateDeviceRGB());
    auto context = ScopedTypeRef<CGContextRef>(CGBitmapContextCreate(
        bitmap_data.data(), width, height, 8, width * 4, color_space_ref.get(),
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetAllowsFontSmoothing(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), FontSmoothingEnabled());
    CGContextSetAllowsFontSubpixelQuantization(context.get(), true);
    CGContextSetShouldSubpixelQuantizeFonts(context.get(), true);
    CGContextSetAllowsFontSubpixelPositioning(context.get(), true);
    CGContextSetShouldSubpixelPositionFonts(context.get(), true);
    CGContextSetAllowsAntialiasing(context.get(), true);
    CGContextSetShouldAntialias(context.get(), true);

    CGContextSetRGBFillColor(context.get(), 1.0, 1.0, 1.0, 1.0);

    // TODO: Why do we not offset by `-left` here?
    CGPoint rasterization_origin = CGPointMake(0, descent);
    CGContextScaleCTM(context.get(), scale_factor, scale_factor);

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context.get());

    // If the font is a color font and the glyph doesn't have an outline, it is a color glyph.
    // https://github.com/sublimehq/sublime_text/issues/3747#issuecomment-726837744
    bool colored_font = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;
    bool has_outline = CTFontCreatePathForGlyph(font_ref, glyph_index, nullptr);
    bool colored = colored_font && !has_outline;

    return {
        .left = left,
        .top = top,
        .width = static_cast<int32_t>(width),
        .height = static_cast<int32_t>(height),
        .buffer = std::move(bitmap_data),
        .colored = colored,
    };
}

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#195
LineLayout FontRasterizer::layout_line(FontId font_id, std::string_view str8) {
    assert(str8.find('\n') == std::string_view::npos);

    unicode::UTF16ToUTF8IndicesMap indices_map;
    if (!indices_map.set_utf8(str8.data(), str8.length())) {
        fmt::println("UTF16ToUTF8IndicesMap::setUTF8 error");
        std::abort();
    }

    CTFontRef ct_font = font_id_to_native[font_id].font.get();
    auto ct_line = ScopedCFTypeRef<CTLineRef>(CreateCTLine(ct_font, font_id, str8));

    int total_advance = 0;
    std::vector<ShapedGlyph> glyphs;
    CFArrayRef run_array = CTLineGetGlyphRuns(ct_line.get());
    CFIndex run_count = CFArrayGetCount(run_array);

    for (CFIndex i = 0; i < run_count; ++i) {
        CTRunRef ct_run = static_cast<CTRunRef>(CFArrayGetValueAtIndex(run_array, i));

        auto ct_font = static_cast<CTFontRef>(
            CFDictionaryGetValue(CTRunGetAttributes(ct_run), kCTFontAttributeName));
        auto scoped_ct_font =
            ScopedCFTypeRef<CTFontRef>(ct_font, base::apple::OwnershipPolicy::RETAIN);
        int font_size = CTFontGetSize(ct_font);
        FontId run_font_id = cache_font({std::move(scoped_ct_font)}, font_size);

        CFIndex glyph_count = CTRunGetGlyphCount(ct_run);
        std::vector<CGGlyph> glyph_ids(glyph_count);
        std::vector<CFIndex> indices(glyph_count);
        std::vector<CGPoint> positions(glyph_count);
        std::vector<CGSize> advances(glyph_count);

        CTRunGetGlyphs(ct_run, {0, glyph_count}, glyph_ids.data());
        CTRunGetStringIndices(ct_run, {0, glyph_count}, indices.data());
        CTRunGetPositions(ct_run, {0, glyph_count}, positions.data());
        CTRunGetAdvances(ct_run, {0, glyph_count}, advances.data());

        // TODO: Don't hard-code scale factor.
        int scale_factor = 2;

        for (CFIndex i = 0; i < glyph_count; ++i) {
            // TODO: Use subpixel variants instead of rounding.
            Point position = {
                .x = total_advance,
                // TODO: Do we scale y here?
                .y = static_cast<int>(std::ceil(positions[i].y)),
            };
            Point advance = {
                .x = static_cast<int>(std::ceil(advances[i].width * scale_factor)),
                .y = static_cast<int>(std::ceil(advances[i].height * scale_factor)),
            };

            size_t utf8_index = indices_map.map_index(indices[i]);
            ShapedGlyph glyph = {
                .font_id = run_font_id,
                .glyph_id = glyph_ids[i],
                .position = std::move(position),
                .advance = std::move(advance),
                .index = utf8_index,
            };
            glyphs.emplace_back(std::move(glyph));

            total_advance += advance.x;
        }
    }

    return {
        .layout_font_id = font_id,
        .width = total_advance,
        .length = str8.length(),
        .glyphs = std::move(glyphs),
    };
}

FontId FontRasterizer::cache_font(NativeFontType font, int font_size) {
    CTFontRef ct_font = font.font.get();
    auto ct_font_name = ScopedCFTypeRef<CFStringRef>(CTFontCopyPostScriptName(ct_font));
    std::string font_name = base::apple::CFStringToString(ct_font_name.get());

    if (font_name.empty()) {
        fmt::println("FontRasterizer::cacheFont() error: font_name is empty");
        std::abort();
    }

    // If the font is already present, return its ID.
    size_t hash = hash_font(font_name, font_size);
    if (auto it = font_hash_to_id.find(hash); it != font_hash_to_id.end()) {
        return it->second;
    }

    // TODO: Don't hard-code scale factor.
    int scale_factor = 2;

    int ascent = std::ceil(CTFontGetAscent(ct_font)) * scale_factor;
    int descent = std::ceil(CTFontGetDescent(ct_font)) * scale_factor;
    int leading = std::ceil(CTFontGetLeading(ct_font)) * scale_factor;

    int line_height = ascent + descent + leading;

    Metrics metrics = {
        .line_height = line_height,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };

    FontId font_id = font_hash_to_id.size();
    font_hash_to_id.emplace(hash, font_id);
    font_id_to_native.emplace_back(std::move(font));
    font_id_to_metrics.emplace_back(std::move(metrics));
    font_id_to_postscript_name.emplace_back(std::move(font_name));
    return font_id;
}

namespace {

ScopedCFTypeRef<CTLineRef> CreateCTLine(CTFontRef ct_font, FontId font_id, std::string_view str8) {
    auto cf_str = base::apple::StringToCFStringNoCopy(str8);

    auto attr = ScopedCFTypeRef<CFMutableDictionaryRef>(CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ct_font);

    auto attr_string = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, cf_str.get(), attr.get()));

    return CTLineCreateWithAttributedString(attr_string.get());
}

// https://github.com/alacritty/crossfont/blob/9cd8ed05c9cc7ec17fe69183912560f97c050a1a/src/darwin/mod.rs#L275
bool FontSmoothingEnabled() {
    auto pref = ScopedCFTypeRef<CFPropertyListRef>(
        CFPreferencesCopyAppValue(CFSTR("AppleFontSmoothing"), kCFPreferencesCurrentApplication));

    // Case 0: The preference does not exist. By default, macOS smooths fonts.
    if (!pref.get()) {
        return true;
    }
    // Case 1: The preference is an integer. Anything greater than 0 enables smoothing.
    else if (CFGetTypeID(pref.get()) == CFNumberGetTypeID()) {
        CFNumberRef cfnumber = static_cast<CFNumberRef>(pref.get());
        int value;
        CFNumberGetValue(cfnumber, kCFNumberIntType, &value);
        return value != 0;
    }
    // Case 2: The preference is a string. Parse it as an integer.
    else if (CFGetTypeID(pref.get()) == CFStringGetTypeID()) {
        CFStringRef cfstring = static_cast<CFStringRef>(pref.get());
        int value = CFStringGetIntValue(cfstring);
        return value != 0;
    } else {
        return true;
    }
}

}  // namespace

}  // namespace font
