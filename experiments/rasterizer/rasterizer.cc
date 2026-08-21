#include "experiments/rasterizer/capture.h"
#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/gl_helpers.h"
#include "experiments/rasterizer/layout.h"
#include <algorithm>
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

// The sample text for interactive mode. Most families get a pangram plus Lorem ipsum; a few get a
// corpus that exercises the feature they were added to test (color emoji, stacked marks,
// ligatures/flags). The screenshot suite (--test) reads its text from files instead.
std::vector<std::string> corpus_for(const std::string& family) {
    if (family == "Apple Color Emoji") {
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
    capture::Crop crop{0, 80, 1600, 600};
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

    constexpr double scale = 2.0;  // TODO: Get scale from the display.
    std::vector<TestShot> shots;
    for (const auto& path : texts) {
        std::string stem = path.stem().string();
        std::vector<std::string> lines = split_lines(read_file(path.string()));
        for (const std::string& face : faces) {
            for (const std::string& size : sizes) {
                shots.push_back(
                    {.font = {face, std::stod(size)},
                     .lines = lines,
                     .out_path = out_dir + "/" + stem + "-" + face + "-" + size + ".png"});
            }
        }
    }
    spdlog::info("rendering {} shots -> {}", shots.size(), out_dir);
    run_test_window(std::move(shots), crop, scale);
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    if (argc >= 2 && std::string_view(argv[1]) == "--test") return run_tests(argc, argv);

    // Interactive mode. Starting font; trailing style args, any order: "bold", "italic".
    font::FontSpec spec;
    spec.family = argc > 1 ? argv[1] : "system";
    spec.size = argc > 2 ? std::stod(argv[2]) : 14.0;
    for (int i = 3; i < argc; i++) {
        std::string_view style = argv[i];
        if (style == "bold") spec.weight = font::Weight::Bold;
        else if (style == "italic") spec.slant = font::Slant::Italic;
    }

    constexpr double scale = 2.0;  // TODO: Get scale from the display.

    // Families the window cycles through with [ / ]. The launch family leads so it shows first.
    std::vector<std::string> families = {spec.family};
    for (const char* f : {"system", "Source Code Pro", "Menlo", "Times New Roman",
                          "Apple Color Emoji", "Fira Code"}) {
        if (spec.family != f) families.push_back(f);
    }

    // Re-runs on every font change: pick the family's corpus, build the font, lay it out.
    auto provider = [scale](const font::FontSpec& s) -> GlyphAtlasSource {
        auto handle = font::create_font(s);
        if (!handle) {
            spdlog::error("could not create font \"{}\"", s.family);
            return {};
        }
        return layout_text(*handle, corpus_for(s.family), scale);
    };

    run_text_window(spec, std::move(families), scale, std::move(provider));
}
