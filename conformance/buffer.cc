#include "base/numeric/safe_conversions.h"
#include "build/build_config.h"
#include "conformance/capture.h"
#include "px/gl_render_context.h"
#include "px/px.h"
#include "px/px_offscreen.h"
#include "ui/grapheme_shaper.h"
#include "ui/retained_text.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#if BUILDFLAG(IS_WIN)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

// The conformance captures compare device pixels and run at a 2x backing scale.
constexpr double kScale = 2.0;
constexpr double kWindowWidth = 1728.0;
constexpr double kWindowHeight = 1117.0;
constexpr double kTextLeft = 1.0;  // Aligns the text two device pixels from the left edge.
constexpr double kTextTop = 0.0;
constexpr fcolor kBackground = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kForeground = {0.0f, 0.0f, 0.0f, 1.0f};

#if BUILDFLAG(IS_WIN)
constexpr std::string_view kFacesFilename = "faces-win.txt";
#elif BUILDFLAG(IS_LINUX)
constexpr std::string_view kFacesFilename = "faces-linux.txt";
#else
constexpr std::string_view kFacesFilename = "faces-mac.txt";
#endif

struct TestShot {
    std::string family;
    double size = 0.0;
    uint32_t attrs = PX_FONT_NORMAL;
    std::vector<std::string> lines;
    std::string out_path;
};

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::println("cannot read {}", path);
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::string> split_lines(std::string_view text) {
    if (!text.empty() && text.back() == '\n') {
        text.remove_suffix(1);
    }
    std::vector<std::string> result;
    for (size_t start = 0;;) {
        const size_t newline = text.find('\n', start);
        result.emplace_back(text.substr(start, newline - start));
        if (newline == std::string_view::npos) {
            return result;
        }
        start = newline + 1;
    }
}

std::vector<std::string> read_config(const std::string& path) {
    std::vector<std::string> result;
    for (const std::string& line : split_lines(read_file(path))) {
        const size_t begin = line.find_first_not_of(" \t\r");
        if (begin == std::string::npos || line[begin] == '#') {
            continue;
        }
        result.push_back(line.substr(begin, line.find_last_not_of(" \t\r") - begin + 1));
    }
    return result;
}

retained_text prepare_conformance_line(grapheme_shaper* shaper, std::string_view text) {
    retained_text result;
    double x = 0.0;
    const auto is_whitespace = [](char c) { return c == ' ' || c == '\t'; };
    for (size_t start = 0; start < text.size();) {
        size_t end = start;
        if (is_whitespace(text[end])) {
            while (end < text.size() && is_whitespace(text[end])) {
                ++end;
            }
        } else {
            while (end < text.size() && !is_whitespace(text[end])) {
                ++end;
            }
            while (end < text.size() && is_whitespace(text[end])) {
                ++end;
            }
        }

        retained_text word = prepare_retained_text(shaper, text.substr(start, end - start));
        // Match Sublime's observed line-level culling when a token's logical end is left of the
        // viewport. This matters for an orphan combining-mark run: DirectWrite gives it a negative
        // advance even though its first glyph's bitmap extends right of the run origin.
        if (x + word.advance >= 0.0) {
            for (retained_text_batch& batch : word.batches) {
                batch.x_offset += x;
                for (fx_glyph& glyph : batch.layout.glyphs) {
                    glyph.cluster = base::checked_cast<uint32_t>(start + glyph.cluster);
                }
                result.batches.push_back(std::move(batch));
            }
        }
        x += word.advance;
        start = end;
    }
    result.advance = x;
    return result;
}

class TextPage final : public px_window_event_handler {
public:
    void attach(px_window_t* window) { window_ = window; }

    bool set_content(const TestShot& shot) {
        font_ = px_create_font(shot.family.c_str(), static_cast<float>(shot.size), shot.attrs);
        if (!font_) {
            return false;
        }
        metrics_ = px_font_get_metrics(font_);
        lines_.clear();
        lines_.reserve(shot.lines.size());
        grapheme_shaper* shaper = grapheme_shaper::instance(font_);
        for (std::string_view line : shot.lines) {
            lines_.push_back(prepare_conformance_line(shaper, line));
        }
        if (window_) {
            px_mark_dirty(window_);
        }
        return true;
    }

    bool handle_event(px_event_t*) override { return false; }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        rc->draw_rect(bounds, kBackground);
        if (!font_) {
            return;
        }

        double first_baseline = kTextTop + std::ceil(metrics_.ascent);
#if BUILDFLAG(IS_WIN)
        // ST rounds the unrounded DirectWrite ascent after scaling to device pixels. Rounding the
        // public ascent first creates a repeating one-pixel error as the requested size changes.
        first_baseline =
            kTextTop + std::floor(metrics_.raster_ascent * kScale + 0.4999999999999998) / kScale;
#endif
        rc->begin_text_batch();
        for (size_t line = 0; line < lines_.size(); ++line) {
            draw_retained_text(rc, font_,
                               {kTextLeft, first_baseline + metrics_.line_height * line},
                               kForeground, &lines_[line]);
        }
        rc->end_text_batch();
    }

private:
    px_window_t* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    std::vector<retained_text> lines_;
};

// Whether a headless run still has to bring the platform layer up before capturing. Only Windows
// does: its offscreen surface hangs the GL context off a hidden window, and the window class comes
// from px_init. Metal needs nothing from AppKit, and Linux needs nothing from GTK -- see
// prepare_headless_locale.
bool platform_init_before_headless() {
#if BUILDFLAG(IS_WIN)
    return true;
#else
    return false;
#endif
}

// Matches the one piece of gtk_init that the text actually depends on.
//
// A process starts in the "C" locale no matter what LANG says, and only setlocale moves it.
// pango_language_get_default reads that locale, and the language it derives decides which fallback
// face a complex script resolves to -- so skipping it lays Devanagari and Thai out differently from
// every windowed capture, 128 of 672 shots. gtk_init calls setlocale(LC_ALL, "") from
// setlocale_initialization (gtkmain.c); doing it here gets the same fonts with no GTK, no display
// and no session.
void prepare_headless_locale() {
#if BUILDFLAG(IS_LINUX)
    std::setlocale(LC_ALL, "");
#endif
}

// Renders every shot into an offscreen surface and writes the crop straight out. No window, no run
// loop, no settle loop: a paint is finished when the GPU says so, so each shot is one pass.
int run_headless(const std::vector<TestShot>& shots, capture::Crop crop) {
    px_offscreen_t* surface = px_create_offscreen(kWindowWidth, kWindowHeight, kScale);
    if (!surface) {
        std::println("no offscreen surface on this platform");
        return 1;
    }
    const int width = px_offscreen_width(surface);
    const int height = px_offscreen_height(surface);
    if (crop.w <= 0 || crop.h <= 0) {
        crop = {.x = 0, .y = 0, .w = width, .h = height};
    }
    if (crop.x < 0 || crop.y < 0 || crop.x + crop.w > width || crop.y + crop.h > height) {
        std::println("crop {},{},{},{} does not fit the {}x{} surface", crop.x, crop.y, crop.w,
                     crop.h, width, height);
        px_destroy_offscreen(surface);
        return 2;
    }

    TextPage page;
    bool success = true;
    for (size_t i = 0; i < shots.size(); ++i) {
        const TestShot& shot = shots[i];
        if (!page.set_content(shot)) {
            std::println("skipping {}: could not create font {}", shot.out_path, shot.family);
            success = false;
            continue;
        }
        bool ok = px_offscreen_paint(surface, &page);
        if (ok) {
            const uint32_t* pixels = px_offscreen_pixels(surface) +
                                     static_cast<size_t>(crop.y) * static_cast<size_t>(width) +
                                     static_cast<size_t>(crop.x);
            ok = capture::pixels_to_png(pixels, crop.w, crop.h, width, shot.out_path.c_str());
        }
        std::println("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
        success &= ok;
    }
    px_destroy_offscreen(surface);
    return success ? 0 : 1;
}

int run_tests(int argc, char* argv[]) {
    capture::Crop crop{.x = 0, .y = 0, .w = 1600, .h = 600};
    bool headless = false;
    bool usage_error = argc < 3;
    for (int i = 3; i < argc && !usage_error; ++i) {
        const std::string_view flag = argv[i];
        if (flag == "--headless") {
            headless = true;
        } else if (flag == "--crop" && i + 1 < argc) {
            usage_error = std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w,
                                      &crop.h) != 4;
        } else {
            usage_error = true;
        }
    }
    if (usage_error) {
        std::println(
            "usage: buffer_conformance <tests_dir> <out_dir> [--crop x,y,w,h] [--headless]");
        return 2;
    }

    const std::string tests_dir = argv[1];
    const std::string out_dir = argv[2];
    const std::vector<std::string> faces =
        read_config(tests_dir + "/" + std::string(kFacesFilename));
    const std::vector<std::string> sizes = read_config(tests_dir + "/sizes.txt");

    std::error_code error;
    std::vector<std::filesystem::path> texts;
    for (const auto& entry : std::filesystem::directory_iterator(tests_dir + "/texts", error)) {
        if (entry.path().extension() == ".txt") {
            texts.push_back(entry.path());
        }
    }
    std::sort(texts.begin(), texts.end());
    if (faces.empty() || sizes.empty() || texts.empty()) {
        std::println("no test inputs under {} (need {}, sizes.txt, and texts/*.txt)", tests_dir,
                     kFacesFilename);
        return 2;
    }
    std::filesystem::create_directories(out_dir, error);

    // Sublime turns the glow on through a color scheme's font_style, which the reference plugin
    // cannot vary per shot either, so the whole run is one polarity.
    const uint32_t attrs = std::getenv("BUFFER_GLOW") ? PX_FONT_GLOW : PX_FONT_NORMAL;

    std::vector<TestShot> shots;
    for (const std::filesystem::path& path : texts) {
        const std::string stem = path.stem().string();
        const std::vector<std::string> lines = split_lines(read_file(path.string()));
        for (const std::string& face : faces) {
            for (const std::string& size : sizes) {
                shots.push_back({
                    .family = face,
                    .size = std::stod(size),
                    .attrs = attrs,
                    .lines = lines,
                    .out_path = (std::filesystem::path(out_dir) /
                                 (stem + "-" + face + "-" + size + ".png"))
                                    .string(),
                });
            }
        }
    }
    if (const char* filter = std::getenv("BUFFER_FILTER")) {
        std::erase_if(shots, [filter](const TestShot& shot) {
            return shot.out_path.find(filter) == std::string::npos;
        });
    }
    if (const char* limit_text = std::getenv("BUFFER_LIMIT")) {
        const long limit = std::strtol(limit_text, nullptr, 10);
        if (limit > 0 && static_cast<size_t>(limit) < shots.size()) {
            shots.resize(static_cast<size_t>(limit));
        }
    }
    double hold_seconds = 0.0;
    if (const char* hold_text = std::getenv("BUFFER_HOLD_SECONDS")) {
        hold_seconds = std::max(0.0, std::strtod(hold_text, nullptr));
    }

    std::println("rendering {} shots -> {}", shots.size(), out_dir);
    // The Windows offscreen surface carries its GL context on a hidden window, so the platform
    // layer has to be up there even though nothing is ever shown. Elsewhere it stays down for a
    // headless run: px_init brings up GTK on Linux, which needs the display this path exists to
    // avoid, and NSApplication on macOS.
    if (!headless || platform_init_before_headless()) {
        px_init("buffer-conformance", "com.example.buffer-conformance", argc, argv, 0);
    }
    if (headless) {
        prepare_headless_locale();
        return run_headless(shots, crop);
    }
    TextPage page;
    px_window_t* window = px_create_window(&page, nullptr, kWindowWidth, kWindowHeight,
                                           "buffer conformance", kBackground, 0);
    page.attach(window);
    px_set_window_position(window, vec2{0.0, 0.0});
    // Normal conformance reads the private backing store and does not need focus. A held frame is
    // intended for an external capture probe, so focus it for Mutter's RecordWindow API.
    if (hold_seconds > 0.0) {
        px_show_window(window);
    } else {
        px_show_window_without_focus(window);
    }
    px_mark_dirty(window);

    // Match the original probe: establish an empty, composited baseline before replacing the page.
    for (int i = 0; i < 8; ++i) {
        capture::pump(0.008);
    }
#if BUILDFLAG(IS_WIN)
    const int process_id = _getpid();
#else
    const int process_id = getpid();
#endif
    const capture::WindowId window_id = capture::find_window_for_pid(process_id);
    if (!window_id) {
        std::println("could not find the buffer conformance window");
        px_destroy_window(window);
        return 1;
    }

    capture::Frame baseline = capture::capture_frame(window_id, crop);
    bool success = true;
    for (size_t i = 0; i < shots.size(); ++i) {
        const TestShot& shot = shots[i];
        if (!page.set_content(shot)) {
            std::println("skipping {}: could not create font {}", shot.out_path, shot.family);
            success = false;
            continue;
        }

        capture::Frame settled = capture::wait_settled(window_id, crop, baseline);
        const bool ok = settled && capture::frame_to_png(settled, shot.out_path.c_str());
        std::println("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
        success &= ok;
        if (hold_seconds > 0.0) {
            std::println("holding the rendered window for {} seconds", hold_seconds);
            capture::pump(hold_seconds);
        }
        capture::release_frame(baseline);
        baseline = settled;
    }
    capture::release_frame(baseline);
    px_destroy_window(window);
    return success ? 0 : 1;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::setbuf(stdout, nullptr);
    return run_tests(argc, argv);
}
