#include "experiments/platform/px/px.h"
#include "fx/fx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr double kWindowWidth = 960.0;
constexpr double kWindowHeight = 620.0;
constexpr fcolor kBackground{1.0f, 1.0f, 1.0f, 1.0f};

struct LineSpec {
    std::string_view text;
    float size;
    float glow_radius;
    double baseline;
    fcolor color;
};

constexpr std::array<LineSpec, 4> kLineSpecs = {
    LineSpec{"Coral glow - 16 pt", 16.0f, 2.0f, 90.0, {0.92f, 0.18f, 0.15f, 1.0f}},
    LineSpec{"Ocean glow - 24 pt", 24.0f, 3.0f, 180.0, {0.04f, 0.38f, 0.92f, 1.0f}},
    LineSpec{"Violet glow - 36 pt", 36.0f, 4.0f, 310.0, {0.52f, 0.16f, 0.84f, 1.0f}},
    LineSpec{"Emerald glow - 52 pt", 52.0f, 5.0f, 475.0, {0.0f, 0.52f, 0.29f, 1.0f}},
};

float to_native_font_size(float point_size) {
#if defined(_WIN32)
    return std::floor(point_size * 96.0f / 72.0f + 0.5f);
#else
    return point_size;
#endif
}

fx_glyph_bitmap pad_bitmap(fx_glyph_bitmap bitmap, int padding) {
    if (bitmap.empty() || padding <= 0) {
        return bitmap;
    }

    const size_t padded_width = bitmap.width + static_cast<size_t>(padding * 2);
    const size_t padded_height = bitmap.height + static_cast<size_t>(padding * 2);
    std::vector<uint8_t> padded_pixels(padded_width * padded_height * 4);
    for (size_t y = 0; y < bitmap.height; ++y) {
        const size_t source_offset = y * bitmap.width * 4;
        const size_t destination_offset =
            ((y + static_cast<size_t>(padding)) * padded_width + static_cast<size_t>(padding)) * 4;
        std::memcpy(padded_pixels.data() + destination_offset,
                    bitmap.pixels.data() + source_offset, bitmap.width * 4);
    }

    bitmap.width = padded_width;
    bitmap.height = padded_height;
    bitmap.bearing_x -= padding;
    bitmap.bearing_y -= padding;
    bitmap.pixels = std::move(padded_pixels);
    return bitmap;
}

void draw_bitmap(px_render_context* context,
                 const fx_glyph_bitmap& bitmap,
                 int device_left,
                 int device_top,
                 double scale,
                 fcolor color) {
    const double pixel_size = 1.0 / scale;
    for (size_t y = 0; y < bitmap.height; ++y) {
        for (size_t x = 0; x < bitmap.width; ++x) {
            const size_t offset = (y * bitmap.width + x) * 4;
            const float coverage = static_cast<float>(bitmap.pixels[offset + 3]) / 255.0f;
            if (coverage == 0.0f) {
                continue;
            }
            context->draw_rect(rect{(device_left + static_cast<int>(x)) / scale,
                                    (device_top + static_cast<int>(y)) / scale, pixel_size,
                                    pixel_size},
                               fcolor{color.r, color.g, color.b, color.a * coverage});
        }
    }
}

struct GlowLine {
    LineSpec spec;
    std::unique_ptr<fx_font> font;
    std::unique_ptr<fx_layout> layout;
};

class GlowDemo final : public px_window_event_handler {
public:
    GlowDemo() {
        lines_.reserve(kLineSpecs.size());
        for (const LineSpec& spec : kLineSpecs) {
            std::unique_ptr<fx_font> font =
                fx_create_font("system", to_native_font_size(spec.size), FX_FONT_BOLD);
            std::unique_ptr<fx_layout> layout = font ? font->shape(spec.text) : nullptr;
            lines_.push_back(GlowLine{spec, std::move(font), std::move(layout)});
        }
    }

    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        context->begin_rect_batch();
        context->draw_rect(bounds, kBackground);
        for (const GlowLine& line : lines_) {
            draw_line(context, line);
        }
        context->end_rect_batch();
    }

private:
    static void draw_line(px_render_context* context, const GlowLine& line) {
        if (!line.font || !line.layout) {
            return;
        }

        const double scale = std::max(0.01, std::abs(context->get_scale().x));
        const int padding = static_cast<int>(std::ceil(line.spec.glow_radius * scale)) + 1;
        const double device_origin_x = 64.0 * scale;
        const double device_origin_y = line.spec.baseline * scale;
        fx_glyph_cache cache(line.font.get(), static_cast<float>(scale));

        for (const fx_glyph& glyph : line.layout->glyphs) {
            const double x = device_origin_x + static_cast<double>(glyph.x_offset) * scale;
            const double y = device_origin_y + static_cast<double>(glyph.y_offset) * scale;

            const double fraction = x - std::floor(x);
            const int phase = std::clamp(static_cast<int>(fraction * 6.0), 0, 5);
            fx_glyph_bitmap bitmap =
                cache.lookup_glyph_data(glyph.id, static_cast<unsigned>(phase));
            bitmap = pad_bitmap(std::move(bitmap), padding);
            fx_apply_font_glow(&bitmap, line.spec.glow_radius * static_cast<float>(scale), true);

            draw_bitmap(context, bitmap, static_cast<int>(std::floor(x)) + bitmap.bearing_x,
                        static_cast<int>(std::ceil(y - 0.5)) + bitmap.bearing_y, scale,
                        line.spec.color);
        }
    }

    px_window_t* window_ = nullptr;
    std::vector<GlowLine> lines_;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("glow-demo", "com.example.glow-demo", argc, argv, 0);

    GlowDemo demo;
    px_window_t* window = px_create_window(&demo, nullptr, kWindowWidth, kWindowHeight,
                                           "fx font glow demo", kBackground, PX_WINDOW_DEFAULT);
    if (!window) {
        return 1;
    }
    demo.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
