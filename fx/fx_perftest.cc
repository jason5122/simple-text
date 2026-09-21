#include "fx/fx.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {

using clock_type = std::chrono::steady_clock;

double elapsed_ms(clock_type::time_point start) {
    return std::chrono::duration<double, std::milli>(clock_type::now() - start).count();
}

// The first tens of milliseconds of a fresh process run at a reduced clock, which would otherwise
// land entirely inside the first benchmark and make it look slower than it is. Spinning raises the
// clock; sleeping does not.
void warm_up_clock() {
    const auto start = clock_type::now();
    volatile uint64_t sink = 0;
    while (elapsed_ms(start) < 50.0) {
        for (int i = 0; i < 4096; ++i) {
            sink = sink + static_cast<uint64_t>(i);
        }
    }
}

// Editor-shaped input: many short lines rather than one enormous one, because a layout is built
// per line and shaping cost per line is what the editor actually pays.
std::vector<std::string> make_lines(size_t count) {
    constexpr std::string_view kSamples[] = {
        "The quick brown fox jumps over the lazy dog.",
        "Ünïcödé façade naïve résumé — smart quotes and em dashes.",
        "日本語のテキストと한국어 텍스트が混ざっている行。",
        "std::vector<uint32_t> glyphs;  // ligatures: fi fl ffi -> =>",
        "emoji in a comment: 😀🚀🎉 and then back to plain ASCII",
    };
    std::vector<std::string> lines;
    lines.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        lines.emplace_back(kSamples[i % std::size(kSamples)]);
    }
    return lines;
}

void append_utf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// The rasterization benchmarks need many distinct glyphs, which the repeated sample lines do not
// provide: a screenful of code is only ~100 of them. Sweeping whole blocks also pulls in fallback
// faces, so the cost of rasterizing through a secondary face is represented too.
std::vector<std::string> make_glyph_coverage_lines() {
    std::vector<std::string> lines;
    std::string line;
    const auto flush = [&] {
        if (!line.empty()) {
            lines.push_back(std::move(line));
            line.clear();
        }
    };
    const auto add_range = [&](char32_t first, char32_t last) {
        for (char32_t cp = first; cp <= last; ++cp) {
            append_utf8(line, cp);
            // Keep lines short enough that shaping stays the per-line operation it is in practice.
            if (line.size() >= 64) {
                flush();
            }
        }
        flush();
    };
    add_range(0x0021, 0x007E);  // ASCII
    add_range(0x00A1, 0x024F);  // Latin-1 Supplement and Latin Extended-A/B
    add_range(0x0370, 0x04FF);  // Greek and Cyrillic
    add_range(0x4E00, 0x4EFF);  // CJK unified ideographs
    return lines;
}

std::vector<fx_glyph> shape_all(fx_font& font, std::span<const std::string> lines) {
    std::vector<fx_glyph> glyphs;
    for (const std::string& line : lines) {
        const std::unique_ptr<fx_layout> layout = font.shape(line);
        glyphs.insert(glyphs.end(), layout->glyphs.begin(), layout->glyphs.end());
    }
    return glyphs;
}

std::vector<uint32_t> unique_ids(std::span<const fx_glyph> glyphs) {
    std::unordered_set<uint32_t> seen;
    std::vector<uint32_t> ids;
    for (const fx_glyph& glyph : glyphs) {
        if (seen.insert(glyph.id).second) {
            ids.push_back(glyph.id);
        }
    }
    return ids;
}

void benchmark_shape(fx_font& font, std::span<const std::string> lines) {
    size_t bytes = 0;
    for (const std::string& line : lines) {
        bytes += line.size();
    }

    uint64_t glyph_count = 0;
    const auto start = clock_type::now();
    for (const std::string& line : lines) {
        const std::unique_ptr<fx_layout> layout = font.shape(line);
        glyph_count += layout->glyphs.size();
    }
    const double ms = elapsed_ms(start);
    std::println("shape          {:9.2f} ms  {} lines, {} glyphs, {:.2f} us/line, {:.1f} MB/s", ms,
                 lines.size(), glyph_count, ms * 1000.0 / static_cast<double>(lines.size()),
                 static_cast<double>(bytes) / (ms / 1000.0) / (1024.0 * 1024.0));
}

// A cold cache rasterizes every subpixel phase of every glyph, so this is the cost the editor pays
// the first time a character appears on screen.
void benchmark_rasterize(fx_font& font, std::span<const uint32_t> ids, float scale) {
    fx_glyph_cache cache(font, scale);
    uint64_t tile_pixels = 0;
    const auto start = clock_type::now();
    for (uint32_t id : ids) {
        const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(id);
        for (const fx_glyph_cache::glyph_phase& phase : data.phases) {
            tile_pixels += static_cast<uint64_t>(phase.width) * phase.height;
        }
    }
    const double ms = elapsed_ms(start);
    std::println("rasterize      {:9.2f} ms  {} glyphs x {} phases, {:.1f} us/glyph, {} px cached",
                 ms, ids.size(), fx_glyph_cache::phase_count,
                 ms * 1000.0 / static_cast<double>(ids.size()), tile_pixels);
}

// The per-frame cost once the cache is populated: a hash lookup and a phase selection per glyph.
void benchmark_cache_hit(fx_font& font, std::span<const fx_glyph> glyphs, float scale) {
    fx_glyph_cache cache(font, scale);
    for (const fx_glyph& glyph : glyphs) {
        cache.lookup_glyph_data(glyph.id);
    }

    uint64_t checksum = 0;
    const auto start = clock_type::now();
    for (const fx_glyph& glyph : glyphs) {
        const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(glyph.id);
        // The phase a renderer would pick from the glyph's fractional pen position.
        const size_t phase = static_cast<size_t>(glyph.x_offset * fx_glyph_cache::phase_count) %
                             fx_glyph_cache::phase_count;
        checksum += data.phase_at(phase).width;
    }
    const double ms = elapsed_ms(start);
    std::println("cache hit      {:9.2f} ms  {} lookups, {:.1f} ns/lookup, checksum = {}", ms,
                 glyphs.size(), ms * 1e6 / static_cast<double>(glyphs.size()), checksum);
}

// Everything fx_glyph_cache does for one phase, spelled out: ask for the scratch size and the
// baseline origin inside it, fill the scratch with the background the backend expects, then draw.
// Both steps are mandatory — rasterize() paints into whatever it is handed and never clears, and
// only extents() knows how large that buffer has to be.
uint32_t rasterize_one_phase(fx_font& font,
                             uint32_t glyph,
                             float scale,
                             std::vector<uint32_t>& scratch) {
    vec2 origin;
    vec2 size;
    font.extents(glyph, scale, origin, size);
    // A space has no outline and reports an empty box.
    if (!(size.x > 0.0) || !(size.y > 0.0)) {
        return 0;
    }

    // The spare column catches the antialiased edge once a nonzero phase shifts the glyph right.
    const int width = static_cast<int>(std::ceil(size.x)) + 1;
    const int height = static_cast<int>(std::ceil(size.y));

    // Monochrome glyphs are white ink on opaque black and carry their coverage in the color
    // channels; a color glyph is drawn over transparency and keeps its own colors.
    const bool colored = font.is_color_glyph(glyph);
    const color foreground = color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f);
    const color background = colored ? color::from_normalised(0.0f, 0.0f, 0.0f, 0.0f)
                                     : color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f);

    scratch.assign(static_cast<size_t>(width) * static_cast<size_t>(height),
                   background.packed_argb());
    fx_pixel_buffer buffer{scratch.data(), width, height, width};
    font.rasterize(glyph, origin, scale, &buffer, foreground, 0);
    return scratch[scratch.size() / 2];
}

void benchmark_rasterize_raw(fx_font& font, std::span<const uint32_t> ids, float scale) {
    std::vector<uint32_t> scratch;
    uint64_t checksum = 0;
    const auto start = clock_type::now();
    for (uint32_t id : ids) {
        checksum += rasterize_one_phase(font, id, scale, scratch);
    }
    const double ms = elapsed_ms(start);
    std::println("rasterize raw  {:9.2f} ms  {} glyphs x 1 phase, {:.1f} us/glyph, checksum = {}",
                 ms, ids.size(), ms * 1000.0 / static_cast<double>(ids.size()), checksum);
}

void run_all(std::string_view family, float size, float scale, size_t line_count) {
    const std::unique_ptr<fx_font> font = fx_create_font(family, size, 0);
    if (!font) {
        std::println("could not create font \"{}\"", family);
        return;
    }

    const std::vector<std::string> lines = make_lines(line_count);
    // Untimed: gives the platform shaper a chance to populate its own caches, and produces the
    // glyph stream the cache-hit benchmark replays.
    const std::vector<fx_glyph> glyphs = shape_all(*font, lines);
    const std::vector<uint32_t> ids = unique_ids(shape_all(*font, make_glyph_coverage_lines()));

    // The platform rasterizer keeps its own per-glyph caches for the life of the process, so
    // whichever benchmark ran first would otherwise absorb that cost on behalf of the others.
    // Touching every glyph once leaves all of them measuring steady state; the fx cache each
    // rasterization benchmark builds is still empty.
    {
        std::vector<uint32_t> scratch;
        for (uint32_t id : ids) {
            rasterize_one_phase(*font, id, scale, scratch);
        }
    }

    const fx_font_metrics metrics = font->metrics();
    std::println("--- {} {}px at {}x scale, line height {} ---", family, size, scale,
                 metrics.line_height);
    benchmark_shape(*font, lines);
    benchmark_rasterize(*font, ids, scale);
    benchmark_cache_hit(*font, glyphs, scale);
    benchmark_rasterize_raw(*font, ids, scale);
}

}  // namespace

int main() {
    warm_up_clock();
    run_all("system", 16.0f, 1.0f, 10000);
    run_all("Menlo", 16.0f, 2.0f, 10000);
}
