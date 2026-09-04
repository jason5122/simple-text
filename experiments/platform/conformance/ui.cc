#include "experiments/platform/conformance/capture.h"
#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/px.h"
#include "experiments/platform/ui/retained_text.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

constexpr double kWindowWidth = 300.0;
constexpr double kWindowHeight = 290.0;
// Align the borderless test window with Sublime's sidebar after the standard y=80 Retina crop.
constexpr double kSidebarContentTop = 28.0;
constexpr double kSidebarTopPadding = 10.0;
constexpr double kSidebarLeftPadding = 16.0;
constexpr double kSidebarIndentWidth = 12.0;
constexpr double kSidebarIndentOffset = 5.0;
constexpr double kSidebarRowTopPadding = 3.0;
constexpr double kSidebarRowBottomPadding = 3.0;

constexpr fcolor kSidebarBackground = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kSidebarHeading = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr fcolor kSidebarLabel = {0.0f, 0.0f, 0.0f, 1.0f};

struct TestCase {
    std::string stem;
    std::string face;
    std::string size_label;
    float size = 0.0f;
    std::vector<std::string> labels;
};

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::fprintf(stderr, "cannot read %s\n", path.c_str());
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::string> split_lines(std::string_view text) {
    if (!text.empty() && text.back() == '\n') {
        text.remove_suffix(1);
    }
    std::vector<std::string> result;
    for (size_t start = 0; start < text.size();) {
        const size_t newline = text.find('\n', start);
        std::string line(text.substr(start, newline - start));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        result.push_back(std::move(line));
        if (newline == std::string_view::npos) {
            break;
        }
        start = newline + 1;
    }
    return result;
}

std::string trim(std::string_view text) {
    const size_t begin = text.find_first_not_of(" \t\r");
    if (begin == std::string_view::npos) {
        return {};
    }
    const size_t end = text.find_last_not_of(" \t\r");
    return std::string(text.substr(begin, end - begin + 1));
}

std::vector<std::string> read_config(const std::string& path) {
    std::vector<std::string> result;
    for (const std::string& raw_line : split_lines(read_file(path))) {
        const std::string line = trim(raw_line);
        if (!line.empty() && line[0] != '#') {
            result.push_back(line);
        }
    }
    return result;
}

std::vector<TestCase> read_test_cases(const std::string& tests_dir) {
    std::vector<TestCase> result;
    const std::vector<std::string> faces = read_config(tests_dir + "/faces.txt");
    const std::vector<std::string> sizes = read_config(tests_dir + "/sizes.txt");
    std::error_code error;
    std::vector<std::filesystem::path> text_paths;
    for (const auto& entry : std::filesystem::directory_iterator(tests_dir + "/texts", error)) {
        if (entry.path().extension() == ".txt") {
            text_paths.push_back(entry.path());
        }
    }
    std::sort(text_paths.begin(), text_paths.end());
    if (faces.empty() || sizes.empty() || text_paths.empty()) {
        return {};
    }

    for (const std::filesystem::path& text_path : text_paths) {
        const std::vector<std::string> labels = split_lines(read_file(text_path.string()));
        if (labels.empty()) {
            std::fprintf(stderr, "empty UI test corpus: %s\n", text_path.c_str());
            return {};
        }
        for (const std::string& face : faces) {
            for (const std::string& size_label : sizes) {
                result.push_back({
                    .stem = text_path.stem().string(),
                    .face = face,
                    .size_label = size_label,
                    .size = std::stof(size_label),
                    .labels = labels,
                });
            }
        }
    }
    return result;
}

class SidebarPage final : public px_window_event_handler {
public:
    void attach(px_window_t* window) { window_ = window; }

    bool set_content(const std::vector<std::string>& labels, std::string_view face, float size) {
        font_ = px_create_font(std::string(face).c_str(), size);
        if (!font_) {
            return false;
        }
        metrics_ = px_font_get_metrics(font_);
        heading_layout_ = prepare_retained_text(font_, "FOLDERS");
        label_layouts_.clear();
        label_layouts_.reserve(labels.size());
        for (const std::string& label : labels) {
            retained_text layout = prepare_retained_text(font_, label);
            if (layout.batches.empty()) {
                return false;
            }
            label_layouts_.push_back(std::move(layout));
        }
        if (window_) {
            px_mark_dirty(window_);
        }
        return !heading_layout_.batches.empty();
    }

    bool handle_event(px_event_t*) override { return false; }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        context->draw_rect(bounds, kSidebarBackground);
        if (!font_ || heading_layout_.batches.empty()) {
            return;
        }

        context->begin_text_batch();
        draw_retained_text(context, font_, vec2{kSidebarLeftPadding, text_baseline(0)},
                           kSidebarHeading, &heading_layout_);
        for (size_t i = 0; i < label_layouts_.size(); ++i) {
            draw_retained_text(
                context, font_,
                vec2{kSidebarLeftPadding + kSidebarIndentOffset + kSidebarIndentWidth,
                     text_baseline(i + 1)},
                kSidebarLabel, &label_layouts_[i]);
        }
        context->end_text_batch();
    }

private:
    double row_height() const {
        return kSidebarRowTopPadding + metrics_.line_height + kSidebarRowBottomPadding;
    }

    double text_baseline(size_t index) const {
        return kSidebarContentTop + kSidebarTopPadding + index * row_height() +
               kSidebarRowTopPadding + metrics_.ascent;
    }

    px_window_t* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    retained_text heading_layout_;
    std::vector<retained_text> label_layouts_;
};

int run_tests(int argc, char* argv[]) {
    capture::Crop crop{.x = 0, .y = 80, .w = 600, .h = 500};
    if ((argc != 3 && argc != 5) || (argc == 5 && (std::string_view(argv[3]) != "--crop" ||
                                                   std::sscanf(argv[4], "%d,%d,%d,%d", &crop.x,
                                                               &crop.y, &crop.w, &crop.h) != 4))) {
        std::fprintf(stderr, "usage: ui_conformance <tests_dir> <out_dir> [--crop x,y,w,h]\n");
        return 2;
    }

    const std::string tests_dir = argv[1];
    const std::string out_dir = argv[2];
    const std::vector<TestCase> test_cases = read_test_cases(tests_dir);
    if (test_cases.empty()) {
        std::fprintf(stderr, "no UI test cases found in %s\n", tests_dir.c_str());
        return 2;
    }
    std::error_code error;
    std::filesystem::create_directories(out_dir, error);

    px_init("ui-conformance", "com.example.ui-conformance", argc, argv, 0);
    SidebarPage page;
    px_window_t* window = px_create_window(&page, nullptr, kWindowWidth, kWindowHeight,
                                           "UI conformance", kSidebarBackground, 0);
    page.attach(window);
    px_set_window_position(window, vec2{0.0, 0.0});
    px_show_window_without_focus(window);
    px_mark_dirty(window);

    for (int i = 0; i < 8; ++i) {
        capture::pump(0.008);
    }
    const capture::WindowId window_id = capture::find_window_for_pid(getpid());
    if (!window_id) {
        std::fprintf(stderr, "could not find the UI conformance window\n");
        px_destroy_window(window);
        return 1;
    }

    capture::Frame baseline = capture::capture_frame(window_id, crop);
    int failures = 0;
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const TestCase& test_case = test_cases[i];
        gl_render_context::reset_glyph_atlas_for_testing();
        if (!page.set_content(test_case.labels, test_case.face, test_case.size)) {
            std::fprintf(stderr, "could not create font %s\n", test_case.face.c_str());
            ++failures;
            continue;
        }

        capture::Frame settled = capture::wait_settled(window_id, crop, baseline);
        const std::string out_path = out_dir + "/" + test_case.stem + "-" + test_case.face + "-" +
                                     test_case.size_label + ".png";
        const bool ok = settled && capture::frame_to_png(settled, out_path.c_str());
        std::fprintf(stderr, "[%zu/%zu] %s%s\n", i + 1, test_cases.size(), out_path.c_str(),
                     ok ? "" : "  (FAILED)");
        failures += ok ? 0 : 1;
        capture::release_frame(baseline);
        baseline = settled;
    }
    capture::release_frame(baseline);
    px_destroy_window(window);
    return failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char* argv[]) { return run_tests(argc, argv); }
