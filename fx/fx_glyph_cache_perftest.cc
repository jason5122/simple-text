#include "fx/fx.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
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

uint64_t next_random(uint64_t& state) {
    // splitmix64: cheap, deterministic, and good enough to defeat the prefetcher.
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
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

std::vector<fx_glyph> shape_all(fx_font& font, std::span<const std::string> lines) {
    std::vector<fx_glyph> glyphs;
    for (const std::string& line : lines) {
        const std::unique_ptr<fx_layout> layout = font.shape(line);
        glyphs.insert(glyphs.end(), layout->glyphs.begin(), layout->glyphs.end());
    }
    return glyphs;
}

// What a screenful of source actually looks up: a few dozen distinct glyphs, repeated. This is the
// stream the cache is tuned for, and the one whose lookups stay resident in L1.
std::vector<fx_glyph> make_editor_stream(fx_font& font) {
    constexpr std::string_view kSamples[] = {
        "void fx_glyph_cache::lookup(uint32_t glyph) {",
        "    const auto& data = cache_.find(key)->second;",
        "    // 6 subpixel phases per glyph, cropped to ink.",
        "    return data.phase_at(phase);",
        "}",
    };
    std::vector<std::string> lines;
    for (size_t i = 0; i < 200; ++i) {
        lines.emplace_back(kSamples[i % std::size(kSamples)]);
    }
    return shape_all(font, lines);
}

// Distinct glyph ids, as many as the font and its fallbacks can supply. Sweeping whole blocks also
// reaches secondary faces, whose ids differ in the high half of the key.
std::vector<uint32_t> make_glyph_universe(fx_font& font, size_t target) {
    constexpr std::pair<char32_t, char32_t> kRanges[] = {
        {0x0021, 0x007E},  // ASCII
        {0x00A1, 0x024F},  // Latin-1 Supplement and Latin Extended-A/B
        {0x0370, 0x04FF},  // Greek and Cyrillic
        {0x3040, 0x30FF},  // Hiragana and Katakana
        {0xAC00, 0xAFFF},  // Hangul syllables
        {0x4E00, 0x9FFF},  // CJK unified ideographs
    };

    std::unordered_set<uint32_t> seen;
    std::vector<uint32_t> ids;
    std::string line;
    const auto shape_line = [&] {
        if (line.empty()) return;
        const std::unique_ptr<fx_layout> layout = font.shape(line);
        for (const fx_glyph& glyph : layout->glyphs) {
            if (ids.size() < target && seen.insert(glyph.id).second) {
                ids.push_back(glyph.id);
            }
        }
        line.clear();
    };
    for (const auto& [first, last] : kRanges) {
        for (char32_t cp = first; cp <= last && ids.size() < target; ++cp) {
            append_utf8(line, cp);
            if (line.size() >= 64) {
                shape_line();
            }
        }
        shape_line();
    }
    return ids;
}

// A lookup stream drawn uniformly from `ids`. Materializing it up front keeps the generator out of
// the timed loop; reading it back is sequential, so what the timing isolates is the cache access.
std::vector<uint32_t> make_random_stream(std::span<const uint32_t> ids, size_t length) {
    uint64_t state = 0x243F6A8885A308D3ull;
    std::vector<uint32_t> stream;
    stream.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        stream.push_back(ids[next_random(state) % ids.size()]);
    }
    return stream;
}

size_t cached_pixel_bytes(fx_glyph_cache& cache, std::span<const uint32_t> ids) {
    size_t bytes = 0;
    for (uint32_t id : ids) {
        for (const fx_glyph_cache::glyph_phase& phase : cache.lookup_glyph_data(id).phases) {
            bytes += static_cast<size_t>(phase.width) * phase.height * sizeof(uint32_t);
        }
    }
    return bytes;
}

// Fills a cold cache: every miss rasterizes all subpixel phases through the font and crops each to
// its ink. This is what an editor pays the first time a character reaches the screen.
void benchmark_fill(fx_font& font, std::span<const uint32_t> ids, float scale) {
    fx_glyph_cache cache(font, scale);
    const auto start = clock_type::now();
    for (uint32_t id : ids) {
        cache.lookup_glyph_data(id);
    }
    const double ms = elapsed_ms(start);
    const size_t bytes = cached_pixel_bytes(cache, ids);
    std::println("fill (cold)        {:8.2f} ms  {} glyphs, {:.1f} us/glyph, {} KB of tiles", ms,
                 ids.size(), ms * 1000.0 / static_cast<double>(ids.size()), bytes / 1024);
}

uint64_t time_lookups(fx_glyph_cache& cache,
                      std::span<const uint32_t> stream,
                      size_t repeats,
                      double& ms_out) {
    uint64_t checksum = 0;
    const auto start = clock_type::now();
    for (size_t r = 0; r < repeats; ++r) {
        for (uint32_t id : stream) {
            checksum += cache.lookup_glyph_data(id).phase_at(0).width;
        }
    }
    ms_out = elapsed_ms(start);
    return checksum;
}

void report_lookups(std::string_view label, double ms, size_t lookups, uint64_t checksum) {
    std::println("{:<18} {:8.2f} ms  {} lookups, {:.2f} ns/lookup, checksum = {}", label, ms,
                 lookups, ms * 1e6 / static_cast<double>(lookups), checksum);
}

// The real hot path: a screenful of glyphs, looked up with the phase the pen position selects.
void benchmark_editor_stream(fx_font& font,
                             std::span<const fx_glyph> glyphs,
                             float scale,
                             size_t repeats) {
    fx_glyph_cache cache(font, scale);
    for (const fx_glyph& glyph : glyphs) {
        cache.lookup_glyph_data(glyph.id);
    }

    uint64_t checksum = 0;
    const auto start = clock_type::now();
    for (size_t r = 0; r < repeats; ++r) {
        for (const fx_glyph& glyph : glyphs) {
            const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(glyph.id);
            const size_t phase =
                static_cast<size_t>(glyph.x_offset * fx_glyph_cache::phase_count) %
                fx_glyph_cache::phase_count;
            checksum += data.phase_at(phase).width;
        }
    }
    const double ms = elapsed_ms(start);
    const size_t lookups = glyphs.size() * repeats;
    report_lookups("editor stream", ms, lookups, checksum);

    // A dense screenful is roughly 60 lines of 80 columns; the renderer does one lookup per glyph.
    constexpr size_t kGlyphsPerFrame = 60 * 80;
    std::println("{:<18} {:8.2f} us per {}-glyph frame", "",
                 ms * 1000.0 * kGlyphsPerFrame / static_cast<double>(lookups), kGlyphsPerFrame);
}

// The same map under an access pattern with no locality, which is what isolates the node-per-entry
// layout of the underlying hash map from the L1 residency the editor stream enjoys.
void benchmark_random(fx_font& font,
                      std::span<const uint32_t> ids,
                      float scale,
                      size_t stream_length,
                      size_t repeats) {
    fx_glyph_cache cache(font, scale);
    for (uint32_t id : ids) {
        cache.lookup_glyph_data(id);
    }
    const std::vector<uint32_t> stream = make_random_stream(ids, stream_length);

    double ms = 0.0;
    const uint64_t checksum = time_lookups(cache, stream, repeats, ms);
    report_lookups("random access", ms, stream.size() * repeats, checksum);
}

// How lookup cost tracks the number of distinct glyphs held. An editor stays near the low end; a
// document mixing scripts, or several fonts sharing one cache, climbs it.
//
// The universe is shuffled first and each size takes a prefix of that shuffle. Taking prefixes of
// the code-point order instead would confound size with script: the small sizes would be ASCII out
// of the primary face while the large ones filled up with CJK out of a fallback face, which
// changes both the tile sizes and the spread of the keys.
void benchmark_working_set(fx_font& font, std::span<const uint32_t> universe, float scale) {
    constexpr size_t kSizes[] = {64, 256, 1024, 4096, 16384};
    constexpr size_t kStreamLength = 1 << 20;
    constexpr size_t kRepeats = 8;

    std::vector<uint32_t> shuffled(universe.begin(), universe.end());
    uint64_t state = 0xB5026F5AA96619E9ull;
    for (size_t i = shuffled.size(); i > 1; --i) {
        std::swap(shuffled[i - 1], shuffled[next_random(state) % i]);
    }

    std::println("working set sweep (random access):");
    for (size_t size : kSizes) {
        if (size > shuffled.size()) continue;
        const std::span<const uint32_t> ids = std::span<const uint32_t>{shuffled}.first(size);
        fx_glyph_cache cache(font, scale);
        for (uint32_t id : ids) {
            cache.lookup_glyph_data(id);
        }
        const std::vector<uint32_t> stream = make_random_stream(ids, kStreamLength);

        double ms = 0.0;
        const uint64_t checksum = time_lookups(cache, stream, kRepeats, ms);
        const size_t lookups = stream.size() * kRepeats;
        std::println("  {:6} glyphs   {:8.2f} ms  {:.2f} ns/lookup, {} KB of tiles, checksum = {}",
                     size, ms, ms * 1e6 / static_cast<double>(lookups),
                     cached_pixel_bytes(cache, ids) / 1024, checksum);
    }
}

// Core Text smooths differently over dark and light backgrounds, so a themed editor can touch both
// maps. They are separate tables behind one cache, and this is what alternating between them
// costs.
void benchmark_alternate(fx_font& font,
                         std::span<const uint32_t> ids,
                         float scale,
                         size_t stream_length,
                         size_t repeats) {
    if constexpr (!fx_glyph_cache::alternate_glyphs) {
        return;
    } else {
        fx_glyph_cache cache(font, scale);
        for (uint32_t id : ids) {
            cache.lookup_glyph_data(id, 0, false);
            cache.lookup_glyph_data(id, 0, true);
        }
        const std::vector<uint32_t> stream = make_random_stream(ids, stream_length);

        uint64_t checksum = 0;
        const auto start = clock_type::now();
        for (size_t r = 0; r < repeats; ++r) {
            for (size_t i = 0; i < stream.size(); ++i) {
                checksum += cache.lookup_glyph_data(stream[i], 0, (i & 1) != 0).phase_at(0).width;
            }
        }
        const double ms = elapsed_ms(start);
        report_lookups("alternating maps", ms, stream.size() * repeats, checksum);
    }
}

void run_all(std::string_view family, float size, float scale) {
    const std::unique_ptr<fx_font> font = fx_create_font(family, size, 0);
    if (!font) {
        std::println("could not create font \"{}\"", family);
        return;
    }

    const std::vector<fx_glyph> editor_stream = make_editor_stream(*font);
    const std::vector<uint32_t> universe = make_glyph_universe(*font, 16384);
    // The platform rasterizer keeps its own per-glyph caches for the life of the process, so the
    // fill benchmark would otherwise be charged for warming them on everyone else's behalf.
    {
        fx_glyph_cache warm(*font, scale);
        for (uint32_t id : universe) {
            warm.lookup_glyph_data(id);
        }
    }

    std::println("--- {} {}px at {}x scale, {} phases, {} distinct glyphs ---", family, size,
                 scale, fx_glyph_cache::phase_count, universe.size());
    benchmark_fill(*font, universe, scale);
    benchmark_editor_stream(*font, editor_stream, scale, 2000);
    benchmark_random(*font, universe, scale, 1 << 20, 8);
    benchmark_alternate(*font, std::span<const uint32_t>{universe}.first(1024), scale, 1 << 20, 8);
    benchmark_working_set(*font, universe, scale);
}

}  // namespace

int main() {
    warm_up_clock();
    run_all("system", 16.0f, 1.0f);
    run_all("Menlo", 16.0f, 2.0f);
}
