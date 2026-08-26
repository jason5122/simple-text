#include "build/build_config.h"
#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/gl_helpers.h"
#include "experiments/rasterizer/layout.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Device pixels per DIP. On macOS this is the Retina backing store. On Windows it is measured, not
// derived: Settings reports 300% scaling, but Sublime renders at exactly 2x -- view.em_width()
// returns 14.8447 logical against 29.62 captured. Retune this if the VM's display setup changes.
constexpr double kScale = 2.0;

// Sublime's font_size setting is in points; DirectWrite takes DIPs, and Sublime converts and
// rounds up to a whole DIP before creating the text format. font_size 20 becomes em 27, confirmed
// against view.em_width() reporting 14.8447265625 = 1126/2048 * 27 for Consolas. Core Text takes
// points directly, so macOS passes the size through untouched.
double em_size_for(double points) {
#if BUILDFLAG(IS_WIN)
    return std::ceil(points * 96.0 / 72.0);
#else
    return points;
#endif
}

// The sample text for interactive mode. Most families get a pangram plus Lorem ipsum; a few get a
// corpus that exercises the feature they were added to test (color emoji, stacked marks,
// ligatures/flags). The screenshot suite (--test) reads its text from files instead.
std::vector<std::string> corpus_for(const std::string& family) {
    if (family.find("Emoji") != std::string::npos) {
        return {
            "😀 😃 😄 😁 😆 😅 😂 🤣 🥲 ☺️ 😊 😇 🙂 🙃 😉",
            "😌 😍 🥰 😘 😗 😙 😚 😋 😛 😝 😜 🤪 🤨 🧐 🤓",
            "😎 🤩 🥳 😏 😒 😞 😔 😟 😕 🙁 😣 😖 😫 😩 🥺",
            "😢 😭 😤 😠 😡 🤯 😳 🥵 🥶 😱 😨 😰 😥 😓 🤔",
            "🤫 🤭 🥱 😴 🤤 😷 🤒 🤕 🤢 🤮 🤧 😇 🤠 🤡 🫪",
        };
    }
    if (family == "Geeza Pro") {
        return {
            "", "꣰", "ᩣᩤᩥᩦᩧᩨᩩᩪᩫᩬᩭ", "⃒⃓⃘⃙⃚⃑⃔⃕⃖⃗⃛⃜⃝⃞⃟⃠⃡⃢⃣⃤⃥", "̴̵̶̷̸̡̢̧̨̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̽̾̿̀́͂̓̈́͆͊͋͌ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡Ͱ",
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
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        spdlog::error("cannot read {}", path);
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

// Splits into lines, dropping one trailing newline so a file's final '\n' doesn't add a blank
// line.
std::vector<std::string> split_lines(std::string_view s) {
    if (!s.empty() && s.back() == '\n') s.remove_suffix(1);
    std::vector<std::string> out;
    for (size_t start = 0;;) {
        size_t nl = s.find('\n', start);
        out.emplace_back(s.substr(start, nl - start));
        if (nl == std::string_view::npos) return out;
        start = nl + 1;
    }
}

// One value per line, trimmed, skipping blanks and '#' comments. Used for faces.txt / sizes.txt.
std::vector<std::string> read_config(const std::string& path) {
    std::vector<std::string> out;
    for (const std::string& line : split_lines(read_file(path))) {
        size_t b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos || line[b] == '#') continue;
        out.push_back(line.substr(b, line.find_last_not_of(" \t\r") - b + 1));
    }
    return out;
}

// Renders the screenshot suite: every texts/*.txt x faces.txt x sizes.txt, into <out_dir> as
// <text-stem>-<face>-<size>.png. Sizes are used verbatim as label and value, so the names match
// the Sublime plugin's exactly. Usage: rasterizer --test <tests_dir> <out_dir> [--crop x,y,w,h]
int run_tests(int argc, char* argv[]) {
    // The crop, in device pixels, frames the text region. It sits 56px above capture-sublime.sh's
    // ST crop (0,136): a borderless window has no title/tab bars, so the text starts that much
    // higher. The two crops are the same text region in each app's window.
    Crop crop{0, 80, 1600, 600};
    std::vector<std::string> positionals;
    for (int i = 2; i < argc; i++) {
        std::string_view a = argv[i];
        if (a == "--crop" && i + 1 < argc)
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w, &crop.h);
        else positionals.emplace_back(a);
    }
    if (positionals.size() != 2) {
        spdlog::error("usage: rasterizer --test <tests_dir> <out_dir> [--crop x,y,w,h]");
        return 2;
    }
    const std::string& tests_dir = positionals[0];
    const std::string& out_dir = positionals[1];

    std::vector<std::string> faces = read_config(tests_dir + "/faces.txt");
    std::vector<std::string> sizes = read_config(tests_dir + "/sizes.txt");

    std::error_code ec;
    std::vector<std::filesystem::path> texts;
    for (const auto& e : std::filesystem::directory_iterator(tests_dir + "/texts", ec)) {
        if (e.path().extension() == ".txt") texts.push_back(e.path());
    }
    std::sort(texts.begin(), texts.end());  // directory order is unspecified
    if (faces.empty() || sizes.empty() || texts.empty()) {
        spdlog::error("no test inputs under {} (need faces.txt, sizes.txt, texts/*.txt)",
                      tests_dir);
        return 2;
    }
    std::filesystem::create_directories(out_dir, ec);

    std::vector<TestShot> shots;
    for (const auto& path : texts) {
        std::string stem = path.stem().string();
        std::vector<std::string> lines = split_lines(read_file(path.string()));
        for (const std::string& face : faces) {
            for (const std::string& size : sizes) {
                shots.push_back(
                    {.font = {face, em_size_for(std::stod(size))},
                     .lines = lines,
                     .out_path = out_dir + "/" + stem + "-" + face + "-" + size + ".png"});
            }
        }
    }
    spdlog::info("rendering {} shots -> {}", shots.size(), out_dir);
    run_test_window(std::move(shots), crop, kScale);
    return 0;
}

// Writes the raw output of rasterize() for one glyph as text: the tile geometry and every pixel's
// channels, straight from the backend with no atlas, shader or screenshot in between. That is the
// only way to tell a rasteriser difference from a compositing one -- inverting a screenshot
// assumes the compositing already matches. Usage: rasterizer --dump-glyph <family> <size> <char>
// <phase> <out.txt>
int dump_glyph(int argc, char* argv[]) {
    if (argc < 7 || argc > 8) {
        spdlog::error("usage: rasterizer --dump-glyph <family> <size> <char> <phase> <out.txt> "
                      "[--analysis]");
        return 2;
    }
    font::set_debug_use_analysis_path(argc == 8 && std::string_view(argv[7]) == "--analysis");
    const std::string family = argv[2];
    const double points = std::stod(argv[3]);
    const std::string text = argv[4];
    const int phase = std::stoi(argv[5]);

    auto handle = font::create_font(family, em_size_for(points));
    if (!handle) {
        spdlog::error("could not create font \"{}\"", family);
        return 1;
    }
    const font::ShapedText shaped = font::shape(*handle, text);
    if (shaped.glyphs.empty()) {
        spdlog::error("\"{}\" produced no glyphs", text);
        return 1;
    }
    const font::GlyphPlacement& g = shaped.glyphs[0];

    // Same phase quantisation layout.cc uses, so a dumped tile is one an actual line would place.
    const double subpixel_x = phase * kScale / 6.0;
    const font::GlyphBitmap bmp = font::rasterize(*handle, g.glyph_id, kScale, subpixel_x);

    std::FILE* out = std::fopen(argv[6], "w");
    if (!out) {
        spdlog::error("cannot write {}", argv[6]);
        return 1;
    }
    std::fprintf(out, "family %s\nem %.4f dip  scale %.4f  phase %d  subpixel_x %.6f\n",
                 family.c_str(), handle->size(), kScale, phase, subpixel_x);
    std::fprintf(out, "glyph %u (face %u, index %u)  advance %.6f\n", g.glyph_id,
                 font::face_index_of(g.glyph_id), font::glyph_index_of(g.glyph_id), g.x_advance);
    std::fprintf(out, "rasterizer %s\n", font::rasterizer_debug_info().c_str());
    std::fprintf(out, "tile %zux%zu  bearing %d,%d  colored %d\n", bmp.width, bmp.height,
                 bmp.bearing_x, bmp.bearing_y, bmp.colored ? 1 : 0);
    // Host-order BGRA, so index 2/1/0 are R/G/B -- printed in that order.
    for (size_t y = 0; y < bmp.height; y++) {
        for (size_t x = 0; x < bmp.width; x++) {
            const uint8_t* p = &bmp.pixels[(y * bmp.width + x) * 4];
            std::fprintf(out, "%02x%02x%02x%02x ", p[2], p[1], p[0], p[3]);
        }
        std::fprintf(out, "\n");
    }
    std::fclose(out);
    return 0;
}

// Dumps the same glyph across a grid of gamma and contrast settings, so the pair that reproduces
// Sublime's mask can be found by fitting rather than guessed at. One block per setting.
// Usage: rasterizer --sweep-glyph <family> <size> <char> <phase> <out.txt>
int sweep_glyph(int argc, char* argv[]) {
    if (argc != 7) {
        spdlog::error("usage: rasterizer --sweep-glyph <family> <size> <char> <phase> <out.txt>");
        return 2;
    }
    const std::string family = argv[2];
    const std::string text = argv[4];
    const int phase = std::stoi(argv[5]);
    const double subpixel_x = phase * kScale / 6.0;

    auto handle = font::create_font(family, em_size_for(std::stod(argv[3])));
    if (!handle) {
        spdlog::error("could not create font \"{}\"", family);
        return 1;
    }
    const font::ShapedText shaped = font::shape(*handle, text);
    if (shaped.glyphs.empty()) return 1;
    const font::GlyphId glyph = shaped.glyphs[0].glyph_id;

    std::FILE* out = std::fopen(argv[6], "w");
    if (!out) return 1;
    std::fprintf(out, "family %s\nem %.4f  scale %.4f  phase %d  glyph %u\n", family.c_str(),
                 handle->size(), kScale, phase, glyph);

    for (float gamma : {0.4f, 0.6f, 0.8f, 1.0f, 1.4f, 1.8f, 2.2f}) {
        for (float contrast : {0.0f, 0.5f, 1.0f}) {
            font::set_debug_rendering_params(gamma, contrast);
            const font::GlyphBitmap b = font::rasterize(*handle, glyph, kScale, subpixel_x);
            std::fprintf(out, "block gamma %.2f contrast %.2f tile %zux%zu bearing %d,%d\n", gamma,
                         contrast, b.width, b.height, b.bearing_x, b.bearing_y);
            for (size_t y = 0; y < b.height; y++) {
                for (size_t x = 0; x < b.width; x++) {
                    const uint8_t* q = &b.pixels[(y * b.width + x) * 4];
                    std::fprintf(out, "%02x%02x%02x ", q[2], q[1], q[0]);
                }
                std::fprintf(out, "\n");
            }
        }
    }
    std::fclose(out);
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    if (argc >= 2 && std::string_view(argv[1]) == "--test") return run_tests(argc, argv);
    if (argc >= 2 && std::string_view(argv[1]) == "--dump-glyph") return dump_glyph(argc, argv);
    if (argc >= 2 && std::string_view(argv[1]) == "--sweep-glyph") return sweep_glyph(argc, argv);

    // Interactive mode. Starting font; trailing style args, any order: "bold", "italic". The
    // default family differs per platform because the Windows build is launched from Explorer with
    // no arguments, and it exists to be compared against Sublime Text side by side.
#if BUILDFLAG(IS_WIN)
    constexpr const char* kDefaultFamily = "Consolas";
#else
    constexpr const char* kDefaultFamily = "system";
#endif
    font::FontSpec spec;
    spec.family = argc > 1 ? argv[1] : kDefaultFamily;
    spec.size = em_size_for(argc > 2 ? std::stod(argv[2]) : 20.0);
    for (int i = 3; i < argc; i++) {
        std::string_view style = argv[i];
        if (style == "bold") spec.weight = font::Weight::Bold;
        else if (style == "italic") spec.slant = font::Slant::Italic;
    }

    // Families the window cycles through with [ / ]. The launch family leads so it shows first.
    std::vector<std::string> families = {spec.family};
#if BUILDFLAG(IS_WIN)
    const char* cycle[] = {"Consolas",        "system",         "Cascadia Mono",
                           "Times New Roman", "Segoe UI Emoji", "Fira Code"};
#else
    const char* cycle[] = {"system",          "Source Code Pro",   "Menlo",
                           "Times New Roman", "Apple Color Emoji", "Fira Code"};
#endif
    for (const char* f : cycle) {
        if (spec.family != f) families.push_back(f);
    }

    // Re-runs on every font change: pick the family's corpus, build the font, lay it out.
    auto provider = [](const font::FontSpec& s) -> GlyphAtlasSource {
        auto handle = font::create_font(s);
        if (!handle) {
            spdlog::error("could not create font \"{}\"", s.family);
            return {};
        }
        return layout_text(*handle, corpus_for(s.family), kScale);
    };

    run_text_window(spec, std::move(families), kScale, std::move(provider));
}
