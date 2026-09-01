#include "experiments/platform/conformance/capture/capture.h"
#include "experiments/platform/fx/font_private.h"
#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_font_private.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

// Keep these values in lockstep with the original rasterizer probe and its macOS renderer. The
// capture script compares device pixels and assumes a Retina backing scale.
constexpr double kScale = 2.0;
constexpr double kWindowWidth = 1728.0;
constexpr double kWindowHeight = 1117.0;
constexpr double kTextLeft = 1.0;  // The original renderer's two-device-pixel debug margin.
constexpr double kTextTop = 0.0;
constexpr fcolor kBackground = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kForeground = {0.0f, 0.0f, 0.0f, 1.0f};

#if defined(_WIN32)
constexpr std::string_view kFacesFilename = "faces-win.txt";
#else
constexpr std::string_view kFacesFilename = "faces-mac.txt";
#endif

struct Crop {
    int x = 0;
    int y = 0;
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

        double first_baseline = kTextTop + std::ceil(metrics_.ascent) - scroll_y_;
#if defined(_WIN32)
        // ST rounds the unrounded DirectWrite ascent after scaling to device pixels. Rounding the
        // public ascent first creates a repeating one-pixel error as the requested size changes.
        first_baseline =
            kTextTop +
            std::floor(font_->font->raster_ascent() * kScale + 0.4999999999999998) / kScale -
            scroll_y_;
#endif
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
    size_t limit = 0;
    std::string face_filter;
    std::string size_filter;
    float debug_gamma = -1.0f;
    float debug_contrast = 0.0f;
    float debug_ramp_exponent = -1.0f;
    bool debug_literal_gamma_ramp = false;
    bool debug_inverted_mask = false;
    float debug_cleartype_level = -1.0f;
    bool debug_analysis_path = false;
    bool debug_direct_bitmap = false;
    std::vector<std::string> positionals;
    for (int i = 2; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--crop" && i + 1 < argc) {
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w, &crop.h);
        } else if (argument == "--limit" && i + 1 < argc) {
            limit = std::stoull(argv[++i]);
        } else if (argument == "--face" && i + 1 < argc) {
            face_filter = argv[++i];
        } else if (argument == "--size" && i + 1 < argc) {
            size_filter = argv[++i];
        } else if (argument == "--gamma" && i + 1 < argc) {
            debug_gamma = std::stof(argv[++i]);
        } else if (argument == "--contrast" && i + 1 < argc) {
            debug_contrast = std::stof(argv[++i]);
        } else if (argument == "--ramp-exponent" && i + 1 < argc) {
            debug_ramp_exponent = std::stof(argv[++i]);
        } else if (argument == "--literal-gamma-ramp") {
            debug_literal_gamma_ramp = true;
        } else if (argument == "--inverted-mask") {
            debug_inverted_mask = true;
        } else if (argument == "--cleartype" && i + 1 < argc) {
            debug_cleartype_level = std::stof(argv[++i]);
        } else if (argument == "--analysis") {
            debug_analysis_path = true;
        } else if (argument == "--direct-bitmap") {
            debug_direct_bitmap = true;
        } else {
            positionals.emplace_back(argument);
        }
    }
    if (positionals.size() != 2) {
        std::println("usage: platform_rasterizer --test <tests_dir> <out_dir> "
                     "[--crop x,y,w,h] [--face family] [--size points] [--limit n] "
                     "[--gamma value] [--contrast value] [--ramp-exponent value] "
                     "[--literal-gamma-ramp] [--inverted-mask] [--cleartype value] "
                     "[--analysis]");
        return 2;
    }

    fx_detail::set_debug_rendering_params(debug_gamma, debug_contrast);
    fx_detail::set_debug_gamma_ramp_exponent(debug_ramp_exponent);
    fx_detail::set_debug_literal_gamma_ramp(debug_literal_gamma_ramp);
    fx_detail::set_debug_inverted_mask(debug_inverted_mask);
    fx_detail::set_debug_cleartype_level(debug_cleartype_level);
    fx_detail::set_debug_use_analysis_path(debug_analysis_path);
    fx_detail::set_debug_direct_bitmap(debug_direct_bitmap);

    const std::string& tests_dir = positionals[0];
    const std::string& out_dir = positionals[1];
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
        std::println("no test inputs under {} (need faces.txt, sizes.txt, texts/*.txt)",
                     tests_dir);
        return 2;
    }
    std::filesystem::create_directories(out_dir, error);

    std::vector<TestShot> shots;
    for (const std::filesystem::path& path : texts) {
        const std::string stem = path.stem().string();
        const std::vector<std::string> lines = split_lines(read_file(path.string()));
        for (const std::string& face : faces) {
            if (!face_filter.empty() && face != face_filter) continue;
            for (const std::string& size : sizes) {
                if (!size_filter.empty() && size != size_filter) continue;
                shots.push_back({
                    .font = {.family = face, .size = std::stod(size)},
                    .lines = lines,
                    .out_path = (std::filesystem::path(out_dir) /
                                 (stem + "-" + face + "-" + size + ".png"))
                                    .string(),
                });
            }
        }
    }
    if (limit > 0 && shots.size() > limit) shots.resize(limit);

    std::println("rendering {} shots -> {}", shots.size(), out_dir);
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
#if defined(_WIN32)
    const int process_id = _getpid();
#else
    const int process_id = getpid();
#endif
    const capture::WindowId window_id = capture::find_window_for_pid(process_id);
    if (!window_id) {
        std::println("could not find the platform rasterizer window");
        px_destroy_window(window);
        return 1;
    }

    const capture::Crop capture_crop{crop.x, crop.y, crop.w, crop.h};
    capture::Frame baseline = capture::capture_frame(window_id, capture_crop);
    for (size_t i = 0; i < shots.size(); ++i) {
        const TestShot& shot = shots[i];
        gl_render_context::reset_glyph_atlas_for_testing();
        if (!page.set_content(shot.font, shot.lines)) {
            std::println("skipping {}: could not create font {}", shot.out_path, shot.font.family);
            continue;
        }

        capture::Frame settled = capture::wait_settled(window_id, capture_crop, baseline);
        const bool ok = settled && capture::frame_to_png(settled, shot.out_path.c_str());
        std::println("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
        capture::release_frame(baseline);
        baseline = settled;
    }
    capture::release_frame(baseline);
    px_destroy_window(window);
    return 0;
}

int dump_glyph(int argc, char* argv[]) {
    if (argc < 7 || argc > 8) {
        std::println(
            "usage: platform_rasterizer --dump-glyph <family> <size> <text|@file> <phase> "
            "<out.txt> [--analysis]");
        return 2;
    }
    fx_detail::set_debug_use_analysis_path(argc == 8 && std::string_view(argv[7]) == "--analysis");
    const std::string family = argv[2];
    const double points = std::stod(argv[3]);
    const std::string text_argument = argv[4];
    const std::string text =
        text_argument.starts_with('@') ? read_file(text_argument.substr(1)) : text_argument;
    const int phase = std::stoi(argv[5]);

    px_init("platform-rasterizer", "com.example.platform-rasterizer", argc, argv, 0);
    px_font_t* font = px_create_font(family.c_str(), static_cast<float>(points), PX_FONT_NORMAL);
    if (!font) {
        std::println("could not create font {}", family);
        return 1;
    }
    const std::vector<fx_layout_batch> batches = shape_text_buffer_batches(font, text);
    if (batches.empty() || batches.front().layout.glyphs.empty()) {
        std::println("{} produced no glyphs", text);
        return 1;
    }
    const fx_glyph& glyph = batches.front().layout.glyphs.front();
    const double subpixel_x = phase * kScale / 6.0;
    const fx_glyph_bitmap bitmap = font->font->rasterise(glyph.id, kScale, subpixel_x);

    std::FILE* output = std::fopen(argv[6], "w");
    if (!output) {
        std::println("cannot write {}", argv[6]);
        return 1;
    }
    std::fprintf(output, "family %s\nsize %.4f points  scale %.4f  phase %d  subpixel_x %.6f\n",
                 family.c_str(), points, kScale, phase, subpixel_x);
    std::fprintf(output, "glyph %u  advance %.6f\n", glyph.id, glyph.advance);
    std::fprintf(output, "rasterizer %s\n", fx_detail::rasterizer_debug_info().c_str());
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
        std::println("usage: platform_rasterizer --sweep-glyph <family> <size> <char> <phase> "
                     "<out.txt>");
        return 2;
    }
    const std::string family = argv[2];
    const std::string text = argv[4];
    const int phase = std::stoi(argv[5]);
    const double subpixel_x = phase * kScale / 6.0;
    auto handle = fx_detail::create_font(family, std::stod(argv[3]));
    if (!handle) {
        return 1;
    }
    const fx_detail::ShapedText shaped = fx_detail::shape(*handle, text);
    if (shaped.glyphs.empty()) {
        return 1;
    }
    const fx_detail::GlyphId glyph = shaped.glyphs[0].glyph_id;
    std::FILE* output = std::fopen(argv[6], "w");
    if (!output) {
        return 1;
    }
    std::fprintf(output, "family %s\nem %.4f  scale %.4f  phase %d  glyph %u\n", family.c_str(),
                 handle->size(), kScale, phase, glyph);
    for (float gamma : {0.4f, 0.6f, 0.8f, 1.0f, 1.4f, 1.8f, 2.2f}) {
        for (float contrast : {0.0f, 0.5f, 1.0f}) {
            fx_detail::set_debug_rendering_params(gamma, contrast);
            const fx_detail::GlyphBitmap bitmap =
                fx_detail::rasterize(*handle, glyph, kScale, subpixel_x);
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

int measure_shaping(int argc, char* argv[]) {
    if (argc != 5) {
        std::println("usage: platform_rasterizer --measure-shaping <text.txt> <family> <points>");
        return 2;
    }

    px_init("platform-rasterizer", "com.example.platform-rasterizer", argc, argv, 0);
    const std::string family = argv[3];
    const float points = std::stof(argv[4]);
    px_font_t* font = px_create_font(family.c_str(), points, PX_FONT_NORMAL);
    if (!font) {
        std::println("could not create font {}", family);
        return 1;
    }

    const px_font_metrics metrics = px_font_get_metrics(font);
    std::println("font_face\t{}", family);
    std::println("font_size\t{:.9g}", points);
    std::println("em_width\t{:.9g}", px_font_em_width(font));
    std::println("ascent\t{:.9g}", metrics.ascent);
    std::println("descent\t{:.9g}", metrics.descent);
    std::println("leading\t{:.9g}", metrics.leading);
    std::println("line_height\t{:.9g}", metrics.line_height);

    const std::vector<std::string> lines = split_lines(read_file(argv[2]));
    for (size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const std::vector<fx_layout_batch> batches =
            shape_text_buffer_batches(font, lines[line_index]);
        double advance = 0.0;
        size_t glyph_count = 0;
        for (const fx_layout_batch& batch : batches) {
            advance =
                std::max(advance, batch.x_offset + static_cast<double>(batch.layout.advance));
            glyph_count += batch.layout.glyphs.size();
        }
        std::println("line\t{}\tadvance\t{:.9g}\tglyphs\t{}", line_index, advance, glyph_count);
        for (const fx_layout_batch& batch : batches) {
            for (const fx_glyph& glyph : batch.layout.glyphs) {
                std::println("glyph\t{}\t{}\t{:.9g}\t{:.9g}\t{:.9g}\t{}", line_index, glyph.id,
                             batch.x_offset + glyph.x_offset, glyph.y_offset, glyph.advance,
                             glyph.cluster);
            }
        }
    }
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
    if (argc >= 2 && std::string_view(argv[1]) == "--measure-shaping") {
        return measure_shaping(argc, argv);
    }
    return run_interactive(argc, argv);
}
