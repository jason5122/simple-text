#include "base/numeric/safe_conversions.h"
#include "experiments/platform/conformance/capture.h"
#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/grapheme_shaper.h"
#include "experiments/platform/px/px.h"
#include "experiments/platform/ui/retained_text.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
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

#if defined(_WIN32)
constexpr std::string_view kFacesFilename = "faces-win.txt";
#else
constexpr std::string_view kFacesFilename = "faces-mac.txt";
#endif

struct TestShot {
    std::string family;
    double size = 0.0;
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
        for (retained_text_batch& batch : word.batches) {
            batch.x_offset += x;
            for (fx_glyph& glyph : batch.layout.glyphs) {
                glyph.cluster = base::checked_cast<uint32_t>(start + glyph.cluster);
            }
            result.batches.push_back(std::move(batch));
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
        font_ = px_create_font(shot.family.c_str(), static_cast<float>(shot.size));
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
#if defined(_WIN32)
        // ST rounds the unrounded DirectWrite ascent after scaling to device pixels. Rounding the
        // public ascent first creates a repeating one-pixel error as the requested size changes.
        first_baseline =
            kTextTop +
            std::floor(metrics_.raster_ascent * kScale + 0.4999999999999998) / kScale;
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

int run_tests(int argc, char* argv[]) {
    capture::Crop crop{.x = 0, .y = 0, .w = 1600, .h = 600};
    if ((argc != 3 && argc != 5) || (argc == 5 && (std::string_view(argv[3]) != "--crop" ||
                                                   std::sscanf(argv[4], "%d,%d,%d,%d", &crop.x,
                                                               &crop.y, &crop.w, &crop.h) != 4))) {
        std::println("usage: buffer_conformance <tests_dir> <out_dir> [--crop x,y,w,h]");
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

    std::vector<TestShot> shots;
    for (const std::filesystem::path& path : texts) {
        const std::string stem = path.stem().string();
        const std::vector<std::string> lines = split_lines(read_file(path.string()));
        for (const std::string& face : faces) {
            for (const std::string& size : sizes) {
                shots.push_back({
                    .family = face,
                    .size = std::stod(size),
                    .lines = lines,
                    .out_path = (std::filesystem::path(out_dir) /
                                 (stem + "-" + face + "-" + size + ".png"))
                                    .string(),
                });
            }
        }
    }

    std::println("rendering {} shots -> {}", shots.size(), out_dir);
    px_init("buffer-conformance", "com.example.buffer-conformance", argc, argv, 0);
    TextPage page;
    px_window_t* window = px_create_window(&page, nullptr, kWindowWidth, kWindowHeight,
                                           "buffer conformance", kBackground, 0);
    page.attach(window);
    px_set_window_position(window, vec2{0.0, 0.0});
    // Window-server capture reads the backing store directly, so the suite does not need focus or
    // activation. Keep the borderless test window behind whatever the user is working in.
    px_show_window_without_focus(window);
    px_mark_dirty(window);

    // Match the original probe: establish an empty, composited baseline before replacing the page.
    for (int i = 0; i < 8; ++i) {
        capture::pump(0.008);
    }
#if defined(_WIN32)
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
        gl_render_context::reset_glyph_atlas_for_testing();
        if (!page.set_content(shot)) {
            std::println("skipping {}: could not create font {}", shot.out_path, shot.family);
            success = false;
            continue;
        }

        capture::Frame settled = capture::wait_settled(window_id, crop, baseline);
        const bool ok = settled && capture::frame_to_png(settled, shot.out_path.c_str());
        std::println("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
        success &= ok;
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
