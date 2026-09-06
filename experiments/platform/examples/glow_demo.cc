#include "build/build_config.h"
#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_font_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
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

void draw_glowing_text(px_render_context* context,
                       px_font_t* font,
                       vec2 position,
                       color value,
                       std::string_view text,
                       float radius) {
    const fcolor normalized = value;
    if (!font || !font->font || text.empty() || normalized.a <= 0.0f) {
        return;
    }

    std::unique_ptr<fx_layout> layout = px_shape_text(font, text);
    if (!layout) {
        return;
    }

    const vec2 context_scale = context->get_scale();
    const vec2 translation = context->get_translation();
    const double scale_x = std::max(0.01, std::abs(context_scale.x));
    const double scale_y = std::max(0.01, std::abs(context_scale.y));
    const float device_radius = radius * static_cast<float>(scale_x);
    if (!std::isfinite(device_radius) || device_radius < 2.0f) {
        context->draw_shaped_text(font, position, value, layout.get(), true);
        return;
    }

    const int padding = static_cast<int>(std::ceil(device_radius)) + 1;
    fx_glyph_cache& cache = font->glyph_cache(static_cast<float>(scale_x));
    const double device_origin_x = translation.x + position.x * context_scale.x;
    double device_origin_y = translation.y + position.y * context_scale.y;
#if BUILDFLAG(IS_LINUX)
    device_origin_y -= static_cast<double>(font->font->metrics().ascent) * context_scale.y;
#endif

    context->begin_rect_batch();
    for (const fx_glyph& glyph : layout->glyphs) {
        const double x = device_origin_x + static_cast<double>(glyph.x_offset) * context_scale.x;
        const double y = device_origin_y + static_cast<double>(glyph.y_offset) * context_scale.y;
        const double fraction = x - std::floor(x);
        const int phase = std::clamp(static_cast<int>(fraction * fx_glyph_cache::phase_count), 0,
                                     static_cast<int>(fx_glyph_cache::phase_count) - 1);
        const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(glyph.id);
        const fx_glyph_cache::glyph_phase& source = data.phase_at(phase);
        if (!source.pixels || source.width == 0 || source.height == 0) {
            continue;
        }

        const int width = static_cast<int>(source.width) + padding * 2;
        const int height = static_cast<int>(source.height) + padding * 2;
        std::vector<uint32_t> pixels(static_cast<size_t>(width) * height);
        for (int source_y = 0; source_y < source.height; ++source_y) {
            for (int source_x = 0; source_x < source.width; ++source_x) {
                const uint32_t source_pixel =
                    source.pixels[static_cast<size_t>(source_y) * source.width + source_x];
                const auto* channels = reinterpret_cast<const uint8_t*>(&source_pixel);
                const uint8_t coverage =
                    data.colored ? channels[3] : std::max({channels[0], channels[1], channels[2]});
                pixels[static_cast<size_t>(source_y + padding) * width + source_x + padding] =
                    static_cast<uint32_t>(coverage) * 0x01010101u;
            }
        }

        fx_pixel_buffer buffer{pixels.data(), width, height, width};
        fx_apply_font_glow(&buffer, device_radius, false);

        const int device_left = static_cast<int>(std::floor(x)) + source.bearing_x - padding;
        const int device_top = static_cast<int>(std::ceil(y - 0.5)) + source.bearing_y - padding;
        for (int bitmap_y = 0; bitmap_y < height; ++bitmap_y) {
            for (int bitmap_x = 0; bitmap_x < width; ++bitmap_x) {
                const float coverage =
                    static_cast<float>(pixels[static_cast<size_t>(bitmap_y) * width + bitmap_x] >>
                                       24) /
                    255.0f;
                if (coverage == 0.0f) {
                    continue;
                }
                context->draw_rect(
                    rect{(device_left + bitmap_x - translation.x) / context_scale.x,
                         (device_top + bitmap_y - translation.y) / context_scale.y, 1.0 / scale_x,
                         1.0 / scale_y},
                    fcolor{normalized.r, normalized.g, normalized.b, normalized.a * coverage});
            }
        }
    }
    context->end_rect_batch();

    context->draw_shaped_text(font, position, value, layout.get(), true);
}

class GlowDemo final : public px_window_event_handler {
public:
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
        context->draw_rect(bounds, kBackground);
        for (const LineSpec& line : kLineSpecs) {
            px_font_t* font = px_create_font("system", line.size, PX_FONT_BOLD);
            draw_glowing_text(context, font, {64.0, line.baseline}, line.color, line.text,
                              line.glow_radius);
        }
    }

private:
    px_window_t* window_ = nullptr;
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
