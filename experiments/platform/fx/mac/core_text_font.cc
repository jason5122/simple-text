#include "experiments/platform/fx/fx.h"

#include "experiments/rasterizer/font.h"

#include <cmath>
#include <string>

namespace {

std::string utf32_to_utf8(std::u32string_view input) {
    std::string output;
    output.reserve(input.size());
    for (uint32_t cp : input) {
        if (cp <= 0x7f) {
            output.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0x10ffff) {
            output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return output;
}

fx_glyph_bitmap convert_bitmap(font::GlyphBitmap bitmap) {
    return {
        .width = bitmap.width,
        .height = bitmap.height,
        .bearing_x = bitmap.bearing_x,
        .bearing_y = bitmap.bearing_y,
        .colored = bitmap.colored,
        .pixels = std::move(bitmap.pixels),
    };
}

class core_text_font final : public fx_font {
public:
    core_text_font(font::FontHandle handle, uint32_t attrs)
        : handle_(std::move(handle)), attrs_(attrs) {
        for (size_t i = 0; i < gamma_.values.size(); ++i) {
            gamma_.values[i] = static_cast<uint8_t>(i);
        }
    }

    uint32_t attrs() const override { return attrs_; }

    fx_font_metrics metrics() const override {
        const float ascent = static_cast<float>(std::ceil(handle_.ascent()));
        const float descent = static_cast<float>(std::ceil(handle_.descent()));
        const float leading = static_cast<float>(std::ceil(handle_.leading()));
        return {
            .ascent = ascent,
            .descent = descent,
            .leading = leading,
            .line_height = ascent + descent + leading,
        };
    }

    std::unique_ptr<fx_layout> shape(std::string_view utf8) override {
        const font::ShapedText shaped = font::shape(handle_, utf8);
        auto layout = std::make_unique<fx_layout>();
        layout->advance = shaped.advance;
        layout->line_height = shaped.line_height;
        layout->glyphs.reserve(shaped.glyphs.size());
        for (const font::GlyphPlacement& glyph : shaped.glyphs) {
            layout->glyphs.push_back({
                .id = glyph.glyph_id,
                .advance = glyph.x_advance,
                .x_offset = glyph.x_offset,
                .y_offset = glyph.y_offset,
                .cluster = glyph.cluster,
            });
        }
        return layout;
    }

    std::unique_ptr<fx_layout> shape(std::u32string_view utf32) override {
        return shape(utf32_to_utf8(utf32));
    }

    bool extents(uint32_t glyph, float scale, fx_vec2* origin, fx_vec2* size) override {
        const fx_glyph_bitmap bitmap = rasterise(glyph, scale, 0.0f);
        if (bitmap.empty()) {
            if (origin) {
                *origin = {};
            }
            if (size) {
                *size = {};
            }
            return false;
        }
        if (origin) {
            *origin = {static_cast<float>(bitmap.bearing_x), static_cast<float>(bitmap.bearing_y)};
        }
        if (size) {
            *size = {static_cast<float>(bitmap.width), static_cast<float>(bitmap.height)};
        }
        return true;
    }

    fx_glyph_bitmap rasterise(uint32_t glyph, float scale, float subpixel_x) override {
        return convert_bitmap(font::rasterize(handle_, glyph, scale, subpixel_x));
    }

    bool is_color_glyph(uint32_t glyph) override {
        return font::rasterize(handle_, glyph, 1.0, 0.0).colored;
    }

    // ST returns true on macOS 10.14 and newer. Every macOS version supported by this experiment
    // falls on that side of its compatibility check.
    bool bg_affects_rasterise() const override { return true; }
    const fx_gamma_ramp* gamma_ramp() const override { return &gamma_; }

private:
    font::FontHandle handle_;
    uint32_t attrs_ = 0;
    fx_gamma_ramp gamma_;
};

}  // namespace

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs) {
    std::optional<font::FontHandle> handle = font::create_font(
        std::string(family), size,
        attrs & FX_FONT_BOLD ? font::Weight::Bold : font::Weight::Normal,
        attrs & FX_FONT_ITALIC ? font::Slant::Italic : font::Slant::Normal, attrs);
    if (!handle) {
        return nullptr;
    }
    return std::make_unique<core_text_font>(std::move(*handle), attrs);
}
