#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_font_private.h"

#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/mac/capture.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

// Keep these values in lockstep with //experiments/rasterizer/rasterizer.cc and its macOS
// renderer. The capture script compares device pixels and assumes a Retina backing scale.
constexpr double kScale = 2.0;
constexpr double kWindowWidth = 1728.0;
constexpr double kWindowHeight = 1117.0;
constexpr double kTextLeft = 1.0;  // The original renderer's two-device-pixel debug margin.
constexpr double kTextTop = 34.0;  // Its 68-device-pixel vertical debug margin.
constexpr fcolor kBackground = {252 / 255.0f, 253 / 255.0f, 253 / 255.0f, 1.0f};
constexpr fcolor kForeground = {51 / 255.0f, 51 / 255.0f, 51 / 255.0f, 1.0f};

struct Crop {
    int x = 0;
    int y = 80;
    int w = 1600;
    int h = 600;
};

struct FontSpec {
    std::string family;
    double size = 20.0;
    uint32_t attrs = PX_FONT_NORMAL;
};

struct TestShot {
    FontSpec font;
    std::vector<std::string> lines;
    std::string out_path;
};

std::vector<std::string> corpus_for(const std::string& family) {
    if (family.find("Emoji") != std::string::npos) {
        return {
            "😀 😃 😄 😁 😆 😅 😂 🤣 🥲 ☺️ 😊 😇 🙂 🙃 😉",
            "😌 😍 🥰 😘 😗 😙 😚 😋 😛 😝 😜 🤪 🤨 🧐 🤓",
            "😎 🤩 🥳 😏 😒 😞 😔 😟 😕 🙁 😣 😖 😫 😩 🥺",
            "😢 😭 😤 😠 😡 🤯 😳 🥵 🥶 😱 😨 😰 😥 😓 🤔",
            "🤫 🤭 🥱 😴 🤤 😷 🤒 🤕 🤢 🤮 🤧 😇 🤠 🤡 🧪",
        };
    }
    if (family == "Geeza Pro") {
        return {
            "", "꣰", "ᩣᩤᩥᩦᩧᩨᩩᩪᩫᩬᩭ", "⃒⃓⃘⃙⃚⃑⃔⃕⃖⃗⃛⃜⃝⃞⃟⃠⃡⃢⃣⃤⃥⃦⃨⃧⃩", "̴̵̶̷̸̡̢̧̨̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̽̾̿̀́͂̓̈́͆͊͋͌ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡Ͱ",
        };
    }
    if (family == "Fira Code") {
        return {
            "",
            "fi == !=",
            "🇺🇸 🇯🇵 🇪🇺",
            "👨‍👩‍👧‍👦 🏴‍☠️ 👩‍❤️‍💋‍👨",
            "👍🏽 👩🏽‍🦰 👩🏾‍👨🏼‍👧🏽‍👦🏻",
        };
    }
    return {
        "Sphinx of black quartz, judge my vow!",
        "The quick brown fox jumps over the lazy dog. 你好",
        "",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
    };
}

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

class TextPage final : public px_window_event_handler {
public:
    void attach(px_window_t* window) { window_ = window; }

    bool set_content(const FontSpec& spec, const std::vector<std::string>& lines) {
        font_ = px_create_font(spec.family.c_str(), static_cast<float>(spec.size), spec.attrs);
        lines_ = lines;
        if (!font_) {
            return false;
        }
        metrics_ = px_font_get_metrics(font_);
        if (window_) {
            px_mark_dirty(window_);
        }
        return true;
    }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE &&
            window_) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        (void)dirty;
        (void)dirty_count;
        rc->draw_rect(bounds, kBackground);
        if (!font_) {
            return;
        }

        const double first_baseline = kTextTop + std::ceil(metrics_.ascent) - scroll_y_;
        rc->begin_text_batch();
        for (size_t line = 0; line < lines_.size(); ++line) {
            std::vector<fx_layout_batch> batches = shape_text_buffer_batches(font_, lines_[line]);
            for (fx_layout_batch& batch : batches) {
                rc->draw_shaped_text(
                    font_,
                    vec2{kTextLeft + batch.x_offset, first_baseline + metrics_.line_height * line},
                    kForeground, &batch.layout, true);
            }
        }
        rc->end_text_batch();
    }

    void set_scroll_y(double value) {
        scroll_y_ = std::max(0.0, value);
        if (window_) {
            px_mark_dirty(window_);
        }
    }

    double scroll_y() const { return scroll_y_; }

private:
    px_window_t* window_ = nullptr;
    px_font_t* font_ = nullptr;
    px_font_metrics metrics_;
    std::vector<std::string> lines_;
    double scroll_y_ = 0.0;
};

int run_tests(int argc, char* argv[]) {
    Crop crop;
    std::vector<std::string> positionals;
    for (int i = 2; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--crop" && i + 1 < argc) {
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w, &crop.h);
        } else {
            positionals.emplace_back(argument);
        }
    }
    if (positionals.size() != 2) {
        std::fprintf(stderr, "usage: platform_rasterizer --test <tests_dir> <out_dir> "
                             "[--crop x,y,w,h]\n");
        return 2;
    }

    const std::string& tests_dir = positionals[0];
    const std::string& out_dir = positionals[1];
    const std::vector<std::string> faces = read_config(tests_dir + "/faces.txt");
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
        std::fprintf(stderr, "no test inputs under %s (need faces.txt, sizes.txt, texts/*.txt)\n",
                     tests_dir.c_str());
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
                    .font = {.family = face, .size = std::stod(size)},
                    .lines = lines,
                    .out_path = out_dir + "/" + stem + "-" + face + "-" + size + ".png",
                });
            }
        }
    }

    std::fprintf(stderr, "rendering %zu shots -> %s\n", shots.size(), out_dir.c_str());
    px_init("platform-rasterizer", "com.example.platform-rasterizer", argc, argv, 0);
    TextPage page;
    px_window_t* window = px_create_window(&page, nullptr, kWindowWidth, kWindowHeight,
                                           "platform rasterizer", kBackground, 0);
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
    const uint32_t window_id = capture::find_window_for_pid(getpid());
    if (!window_id) {
        std::fprintf(stderr, "could not find the platform rasterizer window\n");
        px_destroy_window(window);
        return 1;
    }

    const capture::Crop capture_crop{crop.x, crop.y, crop.w, crop.h};
    capture::Frame baseline = capture::capture_frame(window_id, capture_crop);
    for (size_t i = 0; i < shots.size(); ++i) {
        const TestShot& shot = shots[i];
        gl_render_context::reset_glyph_atlas_for_testing();
        if (!page.set_content(shot.font, shot.lines)) {
            std::fprintf(stderr, "skipping %s: could not create font %s\n", shot.out_path.c_str(),
                         shot.font.family.c_str());
            continue;
        }

        capture::Frame settled = capture::wait_settled(window_id, capture_crop, baseline);
        const bool ok = settled && capture::frame_to_png(settled, shot.out_path.c_str());
        std::fprintf(stderr, "[%zu/%zu] %s%s\n", i + 1, shots.size(), shot.out_path.c_str(),
                     ok ? "" : "  (FAILED)");
        capture::release_frame(baseline);
        baseline = settled;
    }
    capture::release_frame(baseline);
    px_destroy_window(window);
    return 0;
}

int dump_glyph(int argc, char* argv[]) {
    if (argc < 7 || argc > 8) {
        std::fprintf(stderr,
                     "usage: platform_rasterizer --dump-glyph <family> <size> <char> <phase> "
                     "<out.txt> [--analysis]\n");
        return 2;
    }
    font::set_debug_use_analysis_path(argc == 8 && std::string_view(argv[7]) == "--analysis");
    const std::string family = argv[2];
    const double points = std::stod(argv[3]);
    const std::string text = argv[4];
    const int phase = std::stoi(argv[5]);

    auto handle = font::create_font(family, points);
    if (!handle) {
        std::fprintf(stderr, "could not create font %s\n", family.c_str());
        return 1;
    }
    const font::ShapedText shaped = font::shape(*handle, text);
    if (shaped.glyphs.empty()) {
        std::fprintf(stderr, "%s produced no glyphs\n", text.c_str());
        return 1;
    }
    const font::GlyphPlacement& glyph = shaped.glyphs[0];
    const double subpixel_x = phase * kScale / 6.0;
    const font::GlyphBitmap bitmap = font::rasterize(*handle, glyph.glyph_id, kScale, subpixel_x);

    std::FILE* output = std::fopen(argv[6], "w");
    if (!output) {
        std::fprintf(stderr, "cannot write %s\n", argv[6]);
        return 1;
    }
    std::fprintf(output, "family %s\nem %.4f dip  scale %.4f  phase %d  subpixel_x %.6f\n",
                 family.c_str(), handle->size(), kScale, phase, subpixel_x);
    std::fprintf(output, "glyph %u (face %u, index %u)  advance %.6f\n", glyph.glyph_id,
                 font::face_index_of(glyph.glyph_id), font::glyph_index_of(glyph.glyph_id),
                 glyph.x_advance);
    std::fprintf(output, "rasterizer %s\n", font::rasterizer_debug_info().c_str());
    std::fprintf(output, "tile %zux%zu  bearing %d,%d  colored %d\n", bitmap.width, bitmap.height,
                 bitmap.bearing_x, bitmap.bearing_y, bitmap.colored ? 1 : 0);
    for (size_t y = 0; y < bitmap.height; ++y) {
        for (size_t x = 0; x < bitmap.width; ++x) {
            const uint8_t* pixel = &bitmap.pixels[(y * bitmap.width + x) * 4];
            std::fprintf(output, "%02x%02x%02x%02x ", pixel[2], pixel[1], pixel[0], pixel[3]);
        }
        std::fprintf(output, "\n");
    }
    std::fclose(output);
    return 0;
}

int sweep_glyph(int argc, char* argv[]) {
    if (argc != 7) {
        std::fprintf(stderr,
                     "usage: platform_rasterizer --sweep-glyph <family> <size> <char> <phase> "
                     "<out.txt>\n");
        return 2;
    }
    const std::string family = argv[2];
    const std::string text = argv[4];
    const int phase = std::stoi(argv[5]);
    const double subpixel_x = phase * kScale / 6.0;
    auto handle = font::create_font(family, std::stod(argv[3]));
    if (!handle) {
        return 1;
    }
    const font::ShapedText shaped = font::shape(*handle, text);
    if (shaped.glyphs.empty()) {
        return 1;
    }
    const font::GlyphId glyph = shaped.glyphs[0].glyph_id;
    std::FILE* output = std::fopen(argv[6], "w");
    if (!output) {
        return 1;
    }
    std::fprintf(output, "family %s\nem %.4f  scale %.4f  phase %d  glyph %u\n", family.c_str(),
                 handle->size(), kScale, phase, glyph);
    for (float gamma : {0.4f, 0.6f, 0.8f, 1.0f, 1.4f, 1.8f, 2.2f}) {
        for (float contrast : {0.0f, 0.5f, 1.0f}) {
            font::set_debug_rendering_params(gamma, contrast);
            const font::GlyphBitmap bitmap = font::rasterize(*handle, glyph, kScale, subpixel_x);
            std::fprintf(output, "block gamma %.2f contrast %.2f tile %zux%zu bearing %d,%d\n",
                         gamma, contrast, bitmap.width, bitmap.height, bitmap.bearing_x,
                         bitmap.bearing_y);
            for (size_t y = 0; y < bitmap.height; ++y) {
                for (size_t x = 0; x < bitmap.width; ++x) {
                    const uint8_t* pixel = &bitmap.pixels[(y * bitmap.width + x) * 4];
                    std::fprintf(output, "%02x%02x%02x ", pixel[2], pixel[1], pixel[0]);
                }
                std::fprintf(output, "\n");
            }
        }
    }
    std::fclose(output);
    return 0;
}

class InteractivePage final : public px_window_event_handler {
public:
    InteractivePage(FontSpec spec, std::vector<std::string> families)
        : spec_(std::move(spec)), families_(std::move(families)) {
        set_content();
    }

    void attach(px_window_t* window) {
        window_ = window;
        page_.attach(window);
        set_content();
    }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_SCROLL) {
            page_.set_scroll_y(page_.scroll_y() - event->scroll_delta.y);
            return true;
        }
        if (event->type != PX_EVENT_KEY || !event->pressed) {
            return false;
        }
        switch (event->key) {
        case PX_KEY_ESCAPE:
            if (window_) {
                px_close_window(window_);
            }
            return true;
        case '-':
            spec_.size = std::max(1.0, spec_.size - 0.5);
            break;
        case '+':
        case '=':
            spec_.size += 0.5;
            break;
        case '[':
            cycle_family(-1);
            break;
        case ']':
            cycle_family(1);
            break;
        case 'b':
            spec_.attrs ^= PX_FONT_BOLD;
            break;
        case 'i':
            spec_.attrs ^= PX_FONT_ITALIC;
            break;
        default:
            return false;
        }
        set_content();
        return true;
    }

    void paint(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        page_.paint(rc, bounds, dirty, dirty_count);
    }

private:
    void cycle_family(int direction) {
        const int count = static_cast<int>(families_.size());
        family_index_ =
            static_cast<size_t>((static_cast<int>(family_index_) + direction + count) % count);
        spec_.family = families_[family_index_];
    }

    void set_content() {
        page_.set_content(spec_, corpus_for(spec_.family));
        if (window_) {
            px_set_window_title(window_, spec_.family.c_str());
        }
    }

    px_window_t* window_ = nullptr;
    TextPage page_;
    FontSpec spec_;
    std::vector<std::string> families_;
    size_t family_index_ = 0;
};

int run_interactive(int argc, char* argv[]) {
    FontSpec spec{.family = argc > 1 ? argv[1] : "system",
                  .size = argc > 2 ? std::stod(argv[2]) : 20.0};
    for (int i = 3; i < argc; ++i) {
        const std::string_view style = argv[i];
        if (style == "bold") {
            spec.attrs |= PX_FONT_BOLD;
        } else if (style == "italic") {
            spec.attrs |= PX_FONT_ITALIC;
        }
    }

    std::vector<std::string> families = {spec.family};
    for (const char* family : {"system", "Source Code Pro", "Menlo", "Times New Roman",
                               "Apple Color Emoji", "Fira Code"}) {
        if (spec.family != family) {
            families.emplace_back(family);
        }
    }

    px_init("platform-rasterizer", "com.example.platform-rasterizer", argc, argv, 0);
    InteractivePage page(std::move(spec), std::move(families));
    px_window_t* window = px_create_window(&page, nullptr, kWindowWidth, kWindowHeight,
                                           "platform rasterizer", kBackground, PX_WINDOW_DEFAULT);
    page.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::setbuf(stdout, nullptr);
    if (argc >= 2 && std::string_view(argv[1]) == "--test") {
        return run_tests(argc, argv);
    }
    if (argc >= 2 && std::string_view(argv[1]) == "--dump-glyph") {
        return dump_glyph(argc, argv);
    }
    if (argc >= 2 && std::string_view(argv[1]) == "--sweep-glyph") {
        return sweep_glyph(argc, argv);
    }
    return run_interactive(argc, argv);
}
