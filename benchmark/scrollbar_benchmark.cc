// Scrollbar-drag smoothness for the editor.
//
// The scene and the drag path are experiments/examples/editor.cc's: the same window_impl ->
// control bridge, the same document, gutter and scrollbar geometry, and the same two routes from
// a mouse event to a frame: per-event (jump_to -> mark_dirty, one commit per mouse event with
// the display link off, the editor's path on a build linked against the macOS 26 SDK) and tick
// (smooth_scroll::track, one frame per display tick, its path elsewhere). The default is the
// editor's path for this build (PX_OS_SMOOTHS_EVENT_FRAMES in px/px.h); --per-event and --tick
// select the other, so the two can be compared on one trace.
// What is added is measurement. Every drag event, paint and presented frame is timed, and the
// presented offsets are compared with the pointer's own trajectory, so the report says how many
// frames the drag produced, how evenly they were spaced, how far each moved against how far the
// pointer had, and how long a pointer position took to reach the glass.
//
//   out/release/scrollbar_benchmark --record /tmp/drag.tsv   drag the thumb yourself; Escape
//   out/release/scrollbar_benchmark --replay /tmp/drag.tsv   the recorded events, from a timed
//                                                            thread inside the process
//
// benchmark/record_scrollbar_drag.sh wraps the first. A trace remembers its window and font
// size, and a replay reproduces them so the thumb sits where it was recorded. Options: --size WxH
// (default: maximized, like the editor), --font-size N (default 16), --log (one stderr line per
// event, paint and presented frame), --per-event and --tick (the drag path).

#include "px/px.h"
#include "ui/retained_text.h"
#include "ui/smooth_scroll.h"
#include "ui/window.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// THE EDITOR'S SCENE, FROM experiments/examples/editor.cc
// ─────────────────────────────────────────────────────────────────────────────────────────────────

constexpr const char* kBodyFontFamily = "Source Code Pro";
constexpr float kMainFontSize = 16.0f;
constexpr float kMinimumFontSize = 2.0f;
constexpr float kMaximumFontSize = 128.0f;
constexpr float kSidebarTitleFontSize = 13.0f;
constexpr float kSidebarFontSize = 12.0f;
constexpr float kStatusBarFontSize = 11.0f;

// Only used when --size is given; the default is maximized, like the editor.
constexpr double kDefaultWindowWidth = 1400.0;
constexpr double kDefaultWindowHeight = 800.0;

constexpr double kSidebarWidth = 250.0;
constexpr double kSidebarTopPadding = 10.0;
constexpr double kSidebarLeftPadding = 16.0;
constexpr double kSidebarIndentWidth = 12.0;
constexpr double kSidebarIndentOffset = 5.0;
constexpr double kSidebarRowTopPadding = 3.0;
constexpr double kSidebarRowBottomPadding = 3.0;
constexpr size_t kSelectedSidebarLine = 1;

constexpr double kGutterLeftPadding = 14.0;
constexpr double kGutterNumberPadding = 3.0;
constexpr double kGutterMiniDiffWidth = 4.0;
constexpr double kGutterMargin = 4.0;
constexpr double kGutterFoldButtonWidth = 13.0;
constexpr int kMinimumLineNumberDigits = 2;
constexpr double kMargin = 1.0;
constexpr double kTextTop = 50.0;

constexpr double kScrollbarTop = 38.0;
constexpr double kScrollbarWidth = 10.0;
constexpr double kScrollbarMargin = 4.0;

constexpr double kMinimumThumbHeight = 36.0;
constexpr double kThumbGrabMargin = 16.0;
constexpr size_t kDocumentLineCount = 500;

constexpr int digit_count(size_t value) {
    int digits = 1;
    for (; value >= 10; value /= 10) {
        ++digits;
    }
    return digits;
}

constexpr int kLineNumberDigits =
    std::max(kMinimumLineNumberDigits, digit_count(kDocumentLineCount));

// The editor shows its find panel at startup, so the document ends where it does here too: the
// panel is part of the geometry the thumb is dragged in.
constexpr double kFindPanelHeight = 40.0;
constexpr double kFindPanelPadding = 8.0;
constexpr double kFindItemHeight = 24.0;
constexpr double kFindItemGap = 6.0;
constexpr double kFindToggleWidth = 28.0;
constexpr double kFindButtonPadding = 12.0;
constexpr double kFindTextInset = 7.0;
constexpr double kTabStripHeight = 32.0;
constexpr double kStatusBarHeight = 22.0;
constexpr double kStatusBarPadding = 16.0;

constexpr fcolor kWindowBackground{1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kSidebarBackground{235.0f / 255.0f, 237.0f / 255.0f, 239.0f / 255.0f, 1.0f};
constexpr fcolor kSidebarSelection{0.86f, 0.91f, 0.98f, 1.0f};
constexpr fcolor kSidebarTitleColor{128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f};
constexpr fcolor kSidebarTextColor{51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f};
constexpr fcolor kFindPanelBackground{199.0f / 255.0f, 203.0f / 255.0f, 209.0f / 255.0f, 1.0f};
constexpr fcolor kStatusBarBackground{199.0f / 255.0f, 203.0f / 255.0f, 209.0f / 255.0f, 1.0f};
constexpr fcolor kTabStripBackground{189.0f / 255.0f, 190.0f / 255.0f, 190.0f / 255.0f, 1.0f};
constexpr fcolor kStatusBarTextColor{64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f};
constexpr fcolor kFindInputBackground{1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kFindButtonBackground{0.985f, 0.985f, 0.99f, 1.0f};
constexpr fcolor kFindLabelColor{0.22f, 0.23f, 0.26f, 1.0f};
constexpr fcolor kFindMutedColor{0.50f, 0.52f, 0.56f, 1.0f};
constexpr fcolor kLineBand{0.86f, 0.91f, 0.98f, 0.45f};
constexpr fcolor kLineNumberColor{152.0f / 255.0f, 152.0f / 255.0f, 152.0f / 255.0f, 1.0f};
constexpr fcolor kScrollbarTrack{0.965f, 0.965f, 0.965f, 1.0f};
constexpr fcolor kScrollbarThumb{0.70f, 0.71f, 0.73f, 1.0f};
constexpr fcolor kScrollbarThumbDragging{0.55f, 0.56f, 0.58f, 1.0f};

constexpr std::array<std::string_view, 4> kFindToggleLabels = {".*", "Aa", "ab", "↩"};
constexpr std::array<std::string_view, 3> kFindButtonLabels = {"Find", "Find Prev", "Find All"};

struct SidebarLine {
    std::string_view text;
    size_t indent_level = 0;
};

constexpr std::array<SidebarLine, 13> kSidebarLines = {
    SidebarLine{"FOLDERS", 0},
    SidebarLine{"User", 1},
    SidebarLine{"temp1", 2},
    SidebarLine{"temp2", 3},
    SidebarLine{"temp3", 4},
    SidebarLine{"ffi", 3},
    SidebarLine{"سلام", 3},
    SidebarLine{"buffer_demo.py", 2},
    SidebarLine{"Default.sublime-commands", 2},
    SidebarLine{"Default.sublime-theme", 2},
    SidebarLine{"Preferences.sublime-settings", 2},
    SidebarLine{"rasterizer_loop.py", 2},
    SidebarLine{"rasterizer_render.py", 2},
};

constexpr std::array<std::string_view, 32> kSourceLines = {
    "namespace editor {",
    "",
    "struct GlyphPosition {",
    "    uint32_t glyph_id = 0;",
    "    float x = 0.0f;",
    "    float advance = 0.0f;",
    "};",
    "",
    "class LineRenderer {",
    "public:",
    "    explicit LineRenderer(FontAtlas* atlas) : atlas_(atlas) {}",
    "",
    "    void draw_line(std::string_view text, vec2 origin) {",
    "        const ShapedLine& line = cache_.shape(text);",
    "        for (const GlyphPosition& glyph : line.glyphs()) {",
    "            const AtlasEntry entry = atlas_->lookup(glyph.glyph_id);",
    "            batch_.push(entry, origin.x + glyph.x, origin.y);",
    "        }",
    "        batch_.flush();",
    "    }",
    "",
    "private:",
    "    // Shaping and rasterization stay cached while scrolling.",
    "    ShapeCache cache_;",
    "    FontAtlas* atlas_ = nullptr;",
    "    GlyphBatch batch_;",
    "};",
    "",
    "constexpr double kLineHeight = 24.0;",
    "constexpr size_t kVisiblePadding = 2;",
    "",
    "}  // Emoji and color glyphs: 👋 🌍 ✨ 🚀",
};

using PreparedText = retained_text;

struct HighlightedRun {
    double x_offset = 0.0;
    fcolor color;
    PreparedText text;
};

using HighlightedLine = std::vector<HighlightedRun>;

constexpr fcolor kPlainText{0.23f, 0.24f, 0.27f, 1.0f};
constexpr fcolor kKeywordText{0.50f, 0.22f, 0.68f, 1.0f};
constexpr fcolor kTypeText{0.10f, 0.40f, 0.67f, 1.0f};
constexpr fcolor kFunctionText{0.13f, 0.46f, 0.55f, 1.0f};
constexpr fcolor kStringText{0.72f, 0.28f, 0.19f, 1.0f};
constexpr fcolor kNumberText{0.76f, 0.38f, 0.08f, 1.0f};
constexpr fcolor kCommentText{0.36f, 0.48f, 0.37f, 1.0f};

constexpr std::array<std::string_view, 11> kKeywords = {
    "class",   "const",   "constexpr", "explicit", "for",  "namespace",
    "nullptr", "private", "public",    "struct",   "void",
};

constexpr std::array<std::string_view, 14> kTypes = {
    "AtlasEntry", "FontAtlas",   "GlyphBatch", "GlyphPosition", "LineRenderer",
    "ShapeCache", "ShapedLine",  "double",     "float",         "size_t",
    "std",        "string_view", "uint32_t",   "vec2",
};

template <size_t Size>
bool contains_token(const std::array<std::string_view, Size>& tokens, std::string_view token) {
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

constexpr bool has_non_ascii(std::string_view text) {
    for (unsigned char c : text) {
        if (c >= 0x80) {
            return true;
        }
    }
    return false;
}

bool is_identifier_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_identifier_continue(char c) { return is_identifier_start(c) || (c >= '0' && c <= '9'); }

HighlightedLine highlight_line(grapheme_shaper* shaper, std::string_view text) {
    HighlightedLine result;
    double x = 0.0;
    auto append = [&](std::string_view token, fcolor color) {
        PreparedText prepared = prepare_retained_text(shaper, token);
        result.push_back({.x_offset = x, .color = color, .text = std::move(prepared)});
        x += result.back().text.advance;
    };

    for (size_t start = 0; start < text.size();) {
        size_t end = start + 1;
        fcolor color = kPlainText;
        char c = text[start];

        if (c == '/' && end < text.size() && text[end] == '/') {
            end = text.size();
            color = kCommentText;
        } else if (c == ' ' || c == '\t') {
            while (end < text.size() && (text[end] == ' ' || text[end] == '\t')) {
                ++end;
            }
        } else if (c == '"') {
            while (end < text.size()) {
                if (text[end] == '\\' && end + 1 < text.size()) {
                    end += 2;
                } else if (text[end++] == '"') {
                    break;
                }
            }
            color = kStringText;
        } else if (c >= '0' && c <= '9') {
            while (end < text.size() && ((text[end] >= '0' && text[end] <= '9') ||
                                         text[end] == '.' || text[end] == 'f')) {
                ++end;
            }
            color = kNumberText;
        } else if (is_identifier_start(c)) {
            while (end < text.size() && is_identifier_continue(text[end])) {
                ++end;
            }
            std::string_view token = text.substr(start, end - start);
            size_t next = end;
            while (next < text.size() && text[next] == ' ') {
                ++next;
            }
            if (contains_token(kKeywords, token)) {
                color = kKeywordText;
            } else if (contains_token(kTypes, token)) {
                color = kTypeText;
            } else if (next < text.size() && text[next] == '(') {
                color = kFunctionText;
            }
        }

        append(text.substr(start, end - start), color);
        start = end;
    }
    return result;
}

void draw_layout(
    px_render_context* context, px_font_t* font, vec2 position, fcolor color, PreparedText* text) {
    draw_retained_text(context, font, position, color, text);
}

void draw_highlighted_line(px_render_context* context,
                           px_font_t* font,
                           vec2 position,
                           HighlightedLine* line) {
    for (HighlightedRun& run : *line) {
        draw_layout(context, font, vec2{position.x + run.x_offset, position.y}, run.color,
                    &run.text);
    }
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// MEASUREMENT
// ─────────────────────────────────────────────────────────────────────────────────────────────────

// A gap this long between drag events is the hand pausing. Frames on either side of it are not
// judged against each other, and the intervals across it are not frame intervals.
constexpr double kPauseSeconds = 0.1;
// A presented step below this is not judged: a slow drag legitimately shows tiny steps.
constexpr double kVisibleStep = 2.0;
// A step this far from the mean of its neighbours is irregular (the report tool's rule).
constexpr double kStepDeviation = 0.35;
constexpr double kLateRefreshes = 1.5;
constexpr double kAssumedRefreshMs = 1000.0 / 120.0;
constexpr double kNominalFrameInterval = 1.0 / 120.0;
// After the last event, for the presentation callbacks of the last frames to arrive.
constexpr int kSettleMs = 300;
// After the window is shown, before an in-process replay starts: the first paint and activation.
constexpr int kReplayLeadMs = 500;
constexpr size_t kMaximumListedFrames = 20;

// ── the trace ───────────────────────────────────────────────────────────────────────────────────
//
// A recorded scrollbar drag: the left button's press, the pointer positions while it is held and
// its release, in window points from the top-left, timed from the first press, with the window
// and body font the drag was recorded in so a replay can put the thumb back where it was. Written
// by --record and played back by --replay.

constexpr char kTraceHeader[] = "# px-drag-trace-v1";

enum class Kind : int {
    press = 0,
    drag = 1,
    release = 2,
};

struct Sample {
    uint64_t time_ns = 0;
    Kind kind = Kind::drag;
    double x = 0.0;
    double y = 0.0;
};

struct Trace {
    std::vector<Sample> samples;
    // Zero when the trace did not say.
    double window_width = 0.0;
    double window_height = 0.0;
    float font_size = 0.0f;
};

bool parse_sample(const std::string& line, Sample* sample) {
    int kind = 0;
    std::istringstream input(line);
    if (!(input >> sample->time_ns >> kind >> sample->x >> sample->y)) {
        return false;
    }
    input >> std::ws;
    if (!input.eof() || kind < 0 || kind > static_cast<int>(Kind::release)) {
        return false;
    }
    sample->kind = static_cast<Kind>(kind);
    return std::isfinite(sample->x) && std::isfinite(sample->y);
}

// "# window 1400x800" and "# font_size 16" carry the recording's geometry. Other comments are
// ignored.
void parse_comment(const std::string& line, Trace* trace) {
    std::istringstream input(line.substr(1));
    std::string key;
    if (!(input >> key)) {
        return;
    }
    if (key == "window") {
        double width = 0.0;
        double height = 0.0;
        char separator = 0;
        if (input >> width >> separator >> height && separator == 'x' && width > 0.0 &&
            height > 0.0) {
            trace->window_width = width;
            trace->window_height = height;
        }
    } else if (key == "font_size") {
        float size = 0.0f;
        if (input >> size && size > 0.0f) {
            trace->font_size = size;
        }
    }
}

bool read_trace(const char* path, Trace* trace, std::string* error) {
    std::ifstream input(path);
    if (!input) {
        *error = std::format("cannot open {}", path);
        return false;
    }
    std::string line;
    bool found_header = false;
    size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line == kTraceHeader) {
            found_header = true;
            continue;
        }
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            parse_comment(line, trace);
            continue;
        }
        Sample sample;
        if (!parse_sample(line, &sample)) {
            *error = std::format("malformed sample at {}:{}", path, line_number);
            return false;
        }
        if (!trace->samples.empty() && sample.time_ns < trace->samples.back().time_ns) {
            *error = std::format("timestamps go backwards at {}:{}", path, line_number);
            return false;
        }
        trace->samples.push_back(sample);
    }
    if (!found_header) {
        *error = std::format("{} is not a version-1 drag trace", path);
        return false;
    }
    if (trace->samples.empty()) {
        *error = std::format("{} contains no samples", path);
        return false;
    }
    return true;
}

bool write_trace(const char* path, const Trace& trace, std::string* error) {
    std::ofstream output(path);
    if (!output) {
        *error = std::format("cannot create {}", path);
        return false;
    }
    output << kTraceHeader << '\n';
    if (trace.window_width > 0.0 && trace.window_height > 0.0) {
        output << std::format("# window {}x{}\n", trace.window_width, trace.window_height);
    }
    if (trace.font_size > 0.0f) {
        output << std::format("# font_size {}\n", trace.font_size);
    }
    output << "# time_ns\tkind\tx\ty\n"
              "# kind: 0 press, 1 drag, 2 release; x and y in window points from the top-left\n";
    for (const Sample& sample : trace.samples) {
        output << std::format("{}\t{}\t{}\t{}\n", sample.time_ns, static_cast<int>(sample.kind),
                              sample.x, sample.y);
    }
    if (!output) {
        *error = std::format("cannot write {}", path);
        return false;
    }
    return true;
}

size_t release_count(const Trace& trace) {
    size_t count = 0;
    for (const Sample& sample : trace.samples) {
        if (sample.kind == Kind::release) {
            ++count;
        }
    }
    return count;
}

// ── timed delivery ──────────────────────────────────────────────────────────────────────────────
//
// Delivers the trace's timestamps on the main thread, independently of the display clock: a
// thread sleeps until each sample is due and posts it over with a zero-delay timeout, which is
// dispatch_after on macOS and safe from any thread. One timer at a time is the point. Queuing a
// timeout per sample up front hands each one leeway proportional to how far ahead it is set,
// hundreds of milliseconds by the end of a trace, which bunches 120 Hz samples into bursts.
//
// The callback receives the time the sample was scheduled for, on the px_now() clock. That plays
// the part of a native event's own timestamp: the time the main thread gets to it is jittered by
// whatever the thread was doing, as with a real event.
void schedule_timed_input(const std::vector<uint64_t>& timestamps_ns,
                          uint64_t phase_ns,
                          std::function<void(size_t index, double scheduled_time)> callback) {
    std::thread([timestamps_ns, phase_ns, callback = std::move(callback)] {
        auto start = std::chrono::steady_clock::now() + std::chrono::nanoseconds(phase_ns);
        double start_seconds = px_now() + static_cast<double>(phase_ns) / 1e9;
        for (size_t index = 0; index < timestamps_ns.size(); ++index) {
            std::this_thread::sleep_until(start + std::chrono::nanoseconds(timestamps_ns[index]));
            double scheduled = start_seconds + static_cast<double>(timestamps_ns[index]) / 1e9;
            px_set_timeout([callback, index, scheduled] { callback(index, scheduled); }, 0);
        }
    }).detach();
}

struct Options {
    const char* record_path = nullptr;
    const char* replay_path = nullptr;
    double width = 0.0;  // 0: the trace's size, else maximized
    double height = 0.0;
    float font_size = 0.0f;  // 0: the trace's size, else kMainFontSize
    bool log = false;
    bool per_event = PX_OS_SMOOTHS_EVENT_FRAMES;
};

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s [--record OUT.tsv | --replay TRACE.tsv] "
                 "[--size WxH] [--font-size N] [--log] [--per-event | --tick]\n",
                 program);
}

bool parse_size(const char* text, double* width, double* height) {
    char* end = nullptr;
    double w = std::strtod(text, &end);
    if (!end || *end != 'x' || w <= 0.0) {
        return false;
    }
    double h = std::strtod(end + 1, &end);
    if (!end || *end != '\0' || h <= 0.0) {
        return false;
    }
    *width = w;
    *height = h;
    return true;
}

bool parse_options(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        const char* value = i + 1 < argc ? argv[i + 1] : nullptr;
        if (std::strcmp(arg, "--record") == 0 && value) {
            options->record_path = value;
            ++i;
        } else if (std::strcmp(arg, "--replay") == 0 && value) {
            options->replay_path = value;
            ++i;
        } else if (std::strcmp(arg, "--size") == 0 && value) {
            if (!parse_size(value, &options->width, &options->height)) {
                return false;
            }
            ++i;
        } else if (std::strcmp(arg, "--font-size") == 0 && value) {
            options->font_size = std::strtof(value, nullptr);
            if (options->font_size < kMinimumFontSize || options->font_size > kMaximumFontSize) {
                return false;
            }
            ++i;
        } else if (std::strcmp(arg, "--log") == 0) {
            options->log = true;
        } else if (std::strcmp(arg, "--per-event") == 0) {
            options->per_event = true;
        } else if (std::strcmp(arg, "--tick") == 0) {
            options->per_event = false;
        } else if (std::strcmp(arg, "--help") == 0) {
            usage(argv[0]);
            std::exit(0);
        } else {
            return false;
        }
    }
    return true;
}

double quantile(std::vector<double> values, double q) {
    std::sort(values.begin(), values.end());
    double index = q * static_cast<double>(values.size() - 1);
    size_t low = static_cast<size_t>(std::floor(index));
    size_t high = static_cast<size_t>(std::ceil(index));
    double fraction = index - static_cast<double>(low);
    return values[low] * (1.0 - fraction) + values[high] * fraction;
}

void print_distribution(const char* name, const char* unit, const std::vector<double>& values) {
    if (values.empty()) {
        std::printf("%s n=0\n", name);
        return;
    }
    std::printf("%s n=%zu p50=%.3f%s p95=%.3f%s p99=%.3f%s max=%.3f%s\n", name, values.size(),
                quantile(values, 0.50), unit, quantile(values, 0.95), unit, quantile(values, 0.99),
                unit, *std::max_element(values.begin(), values.end()), unit);
}

class ScrollbarBenchmark final : public control, public px_application_event_handler {
public:
    ScrollbarBenchmark(window* window,
                       px_window_event_handler* funnel,
                       const Options& options,
                       px_font_t* sidebar_title_font,
                       px_font_t* sidebar_font,
                       float body_font_size)
        : window_(window),
          funnel_(funnel),
          options_(options),
          sidebar_title_font_(sidebar_title_font),
          sidebar_font_(sidebar_font),
          sidebar_title_metrics_(px_font_get_metrics(sidebar_title_font)),
          sidebar_metrics_(px_font_get_metrics(sidebar_font)) {
        grapheme_shaper* ui_shaper = grapheme_shaper::instance(sidebar_font_);
        find_close_layout_ = prepare_retained_text(ui_shaper, "×");
        status_font_ = px_create_font("system", kStatusBarFontSize);
        status_metrics_ = px_font_get_metrics(status_font_);
        status_layout_ = prepare_retained_text(status_font_, "Line 1, Column 1");
        for (size_t i = 0; i < kFindToggleLabels.size(); ++i) {
            find_toggle_layouts_[i] = prepare_retained_text(ui_shaper, kFindToggleLabels[i]);
        }
        for (size_t i = 0; i < kFindButtonLabels.size(); ++i) {
            find_button_layouts_[i] = prepare_retained_text(ui_shaper, kFindButtonLabels[i]);
        }
        for (size_t i = 0; i < kSidebarLines.size(); ++i) {
            px_font_t* font = i == 0 ? sidebar_title_font_ : sidebar_font_;
            sidebar_layouts_[i] = prepare_retained_text(font, kSidebarLines[i].text);
        }
        set_body_font_size(body_font_size);
    }

    float body_font_size() const { return body_font_size_; }

    // --replay: the trace's events, delivered into the window's funnel at their recorded times
    // from a timed thread, the way scroll_benchmark plays a scroll trace.
    void start_replay(Trace trace) {
        replay_ = std::move(trace);
        std::vector<uint64_t> timestamps;
        timestamps.reserve(replay_.samples.size());
        for (const Sample& sample : replay_.samples) {
            timestamps.push_back(sample.time_ns);
        }
        std::printf("replay_started samples=%zu\n", timestamps.size());
        schedule_timed_input(timestamps, 0, [this](size_t index, double scheduled_time) {
            deliver(index, scheduled_time);
        });
    }

    // ── control ─────────────────────────────────────────────────────────────────────────────────

    bool handle_event(const px_event_t* event) override {
        // During an in-process replay the real pointer is still over the window, and a mouseMoved
        // from it while the synthetic button is down would drag the thumb to wherever the hand
        // left the pointer. Only injected input reaches the drag; the keyboard still does.
        if (options_.replay_path && !injecting_ &&
            (event->type == PX_EVENT_MOUSE_BUTTON || event->type == PX_EVENT_MOUSE_MOTION ||
             event->type == PX_EVENT_MOUSE_LEAVE || event->type == PX_EVENT_SCROLL ||
             event->type == PX_EVENT_CAPTURE_LOST)) {
            return true;
        }
        switch (event->type) {
        case PX_EVENT_KEY:
            if (event->pressed && event->key == PX_KEY_ESCAPE) {
                finish();
                return true;
            }
            break;
        case PX_EVENT_SCROLL:
            // The trackpad still scrolls, to get somewhere before a drag. As in the editor.
            if (event->precise_scroll) {
                if (scroll_.scroll(-event->scroll_delta.y, event->timestamp,
                                   maximum_scroll_offset())) {
                    window_->mark_dirty();
                }
            } else {
                jump_scroll_to(scroll_.offset() - event->scroll_delta.y);
            }
            window_->set_animating(scroll_.animating());
            return true;
        case PX_EVENT_MOUSE_BUTTON: {
            if (event->button != PX_MOUSE_LEFT) {
                break;
            }
            if (!event->pressed) {
                if (pressed_in_track_) {
                    note_release(event);
                }
                if (dragging_scrollbar_) {
                    dragging_scrollbar_ = false;
                    window_->mark_dirty();
                    return true;
                }
                break;
            }
            if (!point_in_rect(event->pos, scrollbar_track())) {
                break;
            }
            rect thumb = scrollbar_thumb();
            // As in the editor: on or near the thumb the press takes hold of it where it is;
            // further off the thumb jumps under the pointer first. Either way the drag is on.
            bool near_thumb = event->pos.y >= thumb.y - kThumbGrabMargin &&
                              event->pos.y <= thumb.bottom() + kThumbGrabMargin;
            if (near_thumb) {
                scrollbar_drag_offset_ = event->pos.y - thumb.y;
            } else {
                scrollbar_drag_offset_ = thumb.h * 0.5;
                scroll_thumb_to(event->pos.y);
            }
            dragging_scrollbar_ = true;
            window_->mark_dirty();
            note_press(event, near_thumb);
            return true;
        }
        case PX_EVENT_MOUSE_MOTION:
            if (dragging_scrollbar_) {
                double target = scroll_.offset();
                if (thumb_target(event->pos.y, &target)) {
                    if (options_.per_event) {
                        jump_scroll_to(target);
                    } else {
                        drag_thumb_to(target, event->timestamp);
                    }
                }
                note_drag(event, target);
                return true;
            }
            break;
        case PX_EVENT_CAPTURE_LOST:
            if (dragging_scrollbar_) {
                dragging_scrollbar_ = false;
                note_release(event);
                window_->mark_dirty();
                return true;
            }
            break;
        case PX_EVENT_DESTROY:
            // The close box. Escape and the auto-quit paths have already concluded by now.
            conclude();
            break;
        default:
            break;
        }
        return false;
    }

    void animation_tick(double now) override {
        // A drag runs the display link like a trackpad gesture, one frame per tick sampled from
        // the pointer's trajectory. With --per-event it never runs: jump_to ends the gesture and
        // stops it, and each mouse event commits its own frame.
        double tick_time = px_now();
        if (first_press_time_ != 0.0 && scroll_.animating()) {
            // As in scroll_benchmark: the spread of this is the tick's delivery jitter, which
            // moves the sample point and so the step, since the sample is taken behind the
            // delivery time rather than the display time.
            tick_delays_ms_.push_back((tick_time - (now - kNominalFrameInterval)) * 1000.0);
            if (options_.log) {
                std::fprintf(stderr, "tick target=%.6f now=%.6f offset=%.3f\n", now, tick_time,
                             scroll_.offset());
            }
        }
        if (scroll_.tick(tick_time, maximum_scroll_offset())) {
            window_->mark_dirty();
        }
        window_->set_animating(scroll_.animating());
    }

    void draw(px_render_context* context,
              rect bounds,
              const rect* dirty,
              int dirty_count) override {
        if (!backend_) {
            backend_ = context->supports_batching() ? "gpu-batched" : "software";
        }
        double paint_start = px_now();
        draw_scene(context, bounds);
        double paint_end = px_now();

        // Only frames of the drag count: one carrying drag events, or one the display link drew
        // from the pointer's trajectory (which may fall between two events). Not the press's
        // thumb recolour, not the release's, not a trackpad frame before the drag.
        bool drag_frame = pending_drag_events_ > 0 ||
                          (first_press_time_ != 0.0 && scroll_.offset() != last_painted_offset_);
        last_painted_offset_ = scroll_.offset();
        if (!drag_frame) {
            return;
        }
        paint_ms_.push_back((paint_end - paint_start) * 1000.0);
        if (previous_paint_time_ != 0.0 && previous_paint_run_ == run_) {
            paint_intervals_ms_.push_back((paint_start - previous_paint_time_) * 1000.0);
        }
        previous_paint_time_ = paint_start;
        previous_paint_run_ = run_;
        events_per_frame_.push_back(static_cast<double>(pending_drag_events_));
        uint64_t frame_id = px_current_frame_id(window_->px_window());
        if (frame_id != 0) {
            awaiting_presentation_.insert_or_assign(
                frame_id, painted_frame{.paint_time = paint_start,
                                        .offset = scroll_.offset(),
                                        .last_event_time = last_event_time_,
                                        .run = run_});
        }
        if (options_.log) {
            std::fprintf(stderr, "paint t=%.6f ms=%.3f offset=%.3f events=%zu frame=%llu\n",
                         paint_start, paint_ms_.back(), scroll_.offset(), pending_drag_events_,
                         static_cast<unsigned long long>(frame_id));
        }
        ++drag_paints_;
        pending_drag_events_ = 0;
    }

    // ── px_application_event_handler ───────────────────────────────────────────────────────────

    // Cmd+Q reports too.
    bool can_quit_without_prompt() override {
        conclude();
        return true;
    }

    // ── presentation telemetry ──────────────────────────────────────────────────────────────────

    void frame_presented(uint64_t frame_id, double presented_time) {
        auto found = awaiting_presentation_.find(frame_id);
        if (found == awaiting_presentation_.end()) {
            return;
        }
        painted_frame frame = found->second;
        awaiting_presentation_.erase(found);

        presented_frame shown{.frame_id = frame_id,
                              .time = presented_time,
                              .offset = frame.offset,
                              .run = frame.run};
        if (presented_time <= 0.0) {
            ++never_displayed_;
            presented_.push_back(shown);
            if (options_.log) {
                std::fprintf(stderr, "present never_displayed frame=%llu offset=%.3f\n",
                             static_cast<unsigned long long>(frame_id), frame.offset);
            }
            return;
        }

        if (have_previous_presented_ && previous_presented_.run == frame.run) {
            shown.interval_ms = (presented_time - previous_presented_.time) * 1000.0;
            shown.step = frame.offset - previous_presented_.offset;
            shown.has_step = true;
            presentation_intervals_ms_.push_back(shown.interval_ms);
            presented_steps_.push_back(std::abs(shown.step));
            double ideal_step =
                ideal_offset_at(presented_time) - ideal_offset_at(previous_presented_.time);
            presented_step_errors_.push_back(std::abs(shown.step - ideal_step));
        }
        paint_to_present_ms_.push_back((presented_time - frame.paint_time) * 1000.0);
        if (frame.last_event_time != 0.0 && presented_time >= frame.last_event_time) {
            input_to_present_ms_.push_back((presented_time - frame.last_event_time) * 1000.0);
        }
        position_errors_.push_back(std::abs(frame.offset - ideal_offset_at(presented_time)));
        previous_presented_ = shown;
        have_previous_presented_ = true;
        presented_.push_back(shown);

        if (options_.log) {
            std::fprintf(stderr,
                         "present t=%.6f interval=%.3fms frame=%llu offset=%.3f step=%.3f "
                         "paint_to_present=%.3fms\n",
                         presented_time, shown.interval_ms,
                         static_cast<unsigned long long>(frame_id), frame.offset, shown.step,
                         paint_to_present_ms_.back());
        }
    }

private:
    struct painted_frame {
        double paint_time = 0.0;
        double offset = 0.0;
        double last_event_time = 0.0;
        int run = 0;
    };

    struct presented_frame {
        uint64_t frame_id = 0;
        double time = 0.0;  // 0: never reached the glass
        double offset = 0.0;
        double interval_ms = 0.0;
        double step = 0.0;
        bool has_step = false;  // false for the first frame of a run
        int run = 0;
    };

    struct trajectory_point {
        double time = 0.0;
        double offset = 0.0;
    };

    struct FindPanelLayout {
        rect panel;
        rect close;
        std::array<rect, kFindToggleLabels.size()> toggles;
        rect input;
        std::array<rect, kFindButtonLabels.size()> buttons;
    };

    // ── the drag, as the editor does it ─────────────────────────────────────────────────────────

    double content_bottom() const {
        return window_->size().y - kStatusBarHeight - kFindPanelHeight;
    }

    // As in the editor: from the top of the first line's box, `ascent` above its baseline at
    // kTextTop, to the bottom of the last line's.
    double document_height() const {
        return kTextTop - body_metrics_.ascent + kDocumentLineCount * line_height_;
    }

    double maximum_scroll_offset() const {
        return std::max(0.0, document_height() - content_bottom());
    }

    rect scrollbar_track() const {
        vec2 size = window_->size();
        return rect{size.x - kScrollbarMargin - kScrollbarWidth, kScrollbarTop, kScrollbarWidth,
                    std::max(0.0, content_bottom() - kScrollbarTop - kScrollbarMargin)};
    }

    rect scrollbar_thumb() const {
        rect track = scrollbar_track();
        double viewport_height = content_bottom();
        double thumb_height = std::min(
            track.h, std::max(kMinimumThumbHeight, track.h * viewport_height / document_height()));
        double travel = track.h - thumb_height;
        double maximum_offset = maximum_scroll_offset();
        double progress = maximum_offset > 0.0 ? scroll_.offset() / maximum_offset : 0.0;
        return rect{track.x + 2.0, track.y + travel * progress, track.w - 4.0, thumb_height};
    }

    static bool point_in_rect(vec2 point, rect bounds) {
        return point.x >= bounds.x && point.x < bounds.right() && point.y >= bounds.y &&
               point.y < bounds.bottom();
    }

    void jump_scroll_to(double offset) {
        scroll_.jump_to(offset, maximum_scroll_offset());
        window_->set_animating(false);
        window_->mark_dirty();
    }

    bool thumb_target(double pointer_y, double* offset) const {
        rect track = scrollbar_track();
        rect thumb = scrollbar_thumb();
        double travel = track.h - thumb.h;
        if (travel <= 0.0) {
            return false;
        }
        double thumb_y =
            std::clamp(pointer_y - scrollbar_drag_offset_, track.y, track.bottom() - thumb.h);
        *offset = (thumb_y - track.y) / travel * maximum_scroll_offset();
        return true;
    }

    // A click in the track: the thumb jumps under the pointer.
    void scroll_thumb_to(double pointer_y) {
        double offset = 0.0;
        if (thumb_target(pointer_y, &offset)) {
            jump_scroll_to(offset);
        }
    }

    // The thumb being dragged, as the editor does it now: it jumps under the pointer.
    void drag_thumb_to(double target, double timestamp) { jump_scroll_to(target); }

    // ── bookkeeping around the drag ─────────────────────────────────────────────────────────────

    void note_press(const px_event_t* event, bool grabbed) {
        pressed_in_track_ = true;
        if (first_press_time_ == 0.0) {
            first_press_time_ = event->timestamp;
        }
        ++drag_count_;
        ++run_;
        last_event_time_ = 0.0;
        add_trajectory_point(event->timestamp, scroll_.offset());
        record(Kind::press, event);
        if (options_.log) {
            std::fprintf(stderr, "press t=%.6f x=%.2f y=%.2f grabbed=%d offset=%.3f\n",
                         event->timestamp, event->pos.x, event->pos.y, grabbed ? 1 : 0,
                         scroll_.offset());
        }
    }

    // `target` is the offset the pointer asked for; with the display link in the loop the
    // content reaches it a frame or two later.
    void note_drag(const px_event_t* event, double target) {
        double time = event->timestamp;
        double lag_ms = (px_now() - time) * 1000.0;
        double interval_ms = 0.0;
        event_lags_ms_.push_back(lag_ms);
        if (last_event_time_ != 0.0) {
            if (time - last_event_time_ >= kPauseSeconds) {
                ++run_;
            } else {
                interval_ms = (time - last_event_time_) * 1000.0;
                event_intervals_ms_.push_back(interval_ms);
            }
        }
        last_event_time_ = time;
        ++drag_events_;
        ++pending_drag_events_;
        add_trajectory_point(time, target);
        record(Kind::drag, event);
        if (options_.log) {
            std::fprintf(stderr,
                         "drag t=%.6f dt=%.3fms lag=%.3fms y=%.2f target=%.3f offset=%.3f\n", time,
                         interval_ms, lag_ms, event->pos.y, target, scroll_.offset());
        }
    }

    void note_release(const px_event_t* event) {
        pressed_in_track_ = false;
        ++releases_;
        record(Kind::release, event);
        if (options_.log) {
            std::fprintf(stderr, "release t=%.6f y=%.2f offset=%.3f\n", event->timestamp,
                         event->pos.y, scroll_.offset());
        }
    }

    // The pointer's trajectory in content units: the offset each event asked for, at the
    // event's own time. What a frame presented at time t would ideally show is the trajectory
    // interpolated at t.
    void add_trajectory_point(double time, double offset) {
        if (!trajectory_.empty()) {
            time = std::max(time, trajectory_.back().time);
        }
        trajectory_.push_back(trajectory_point{time, offset});
    }

    double ideal_offset_at(double time) const {
        if (trajectory_.empty()) {
            return scroll_.offset();
        }
        auto after = std::upper_bound(
            trajectory_.begin(), trajectory_.end(), time,
            [](double t, const trajectory_point& point) { return t < point.time; });
        if (after == trajectory_.begin()) {
            return after->offset;
        }
        if (after == trajectory_.end()) {
            return trajectory_.back().offset;
        }
        auto before = std::prev(after);
        if (after->time <= before->time) {
            return after->offset;
        }
        double alpha = (time - before->time) / (after->time - before->time);
        return before->offset + (after->offset - before->offset) * alpha;
    }

    void record(Kind kind, const px_event_t* event) {
        if (!options_.record_path) {
            return;
        }
        double elapsed = std::max(0.0, event->timestamp - first_press_time_);
        uint64_t time_ns = static_cast<uint64_t>(std::llround(elapsed * 1e9));
        if (!recording_.samples.empty()) {
            time_ns = std::max(time_ns, recording_.samples.back().time_ns);
        }
        recording_.samples.push_back(
            Sample{.time_ns = time_ns, .kind = kind, .x = event->pos.x, .y = event->pos.y});
    }

    void deliver(size_t index, double scheduled_time) {
        if (reported_ || index >= replay_.samples.size()) {
            return;
        }
        const Sample& sample = replay_.samples[index];
        px_event_t event{};
        event.window = window_->px_window();
        event.timestamp = scheduled_time;
        event.pos = vec2{sample.x, sample.y};
        if (sample.kind == Kind::drag) {
            event.type = PX_EVENT_MOUSE_MOTION;
        } else {
            event.type = PX_EVENT_MOUSE_BUTTON;
            event.button = PX_MOUSE_LEFT;
            event.pressed = sample.kind == Kind::press;
            event.click_count = 1;
        }
        injecting_ = true;
        funnel_->handle_event(&event);
        injecting_ = false;
        if (index + 1 == replay_.samples.size() && !finishing_) {
            finishing_ = true;
            px_set_timeout([this] { finish(); }, kSettleMs);
        }
    }

    // ── the report ──────────────────────────────────────────────────────────────────────────────

    void finish() {
        conclude();
        window_->close();
    }

    void conclude() {
        write_recording();
        report();
    }

    void write_recording() {
        if (!options_.record_path || recording_written_) {
            return;
        }
        recording_written_ = true;
        if (recording_.samples.empty()) {
            std::printf("nothing recorded: no press landed in the scrollbar track\n");
            return;
        }
        vec2 size = window_->size();
        recording_.window_width = size.x;
        recording_.window_height = size.y;
        recording_.font_size = body_font_size_;
        std::string error;
        if (!write_trace(options_.record_path, recording_, &error)) {
            std::fprintf(stderr, "scrollbar_benchmark: %s\n", error.c_str());
            return;
        }
        std::printf("recorded samples=%zu releases=%zu to %s\n", recording_.samples.size(),
                    release_count(recording_), options_.record_path);
    }

    struct flagged_frame {
        size_t index = 0;
        std::string flags;
    };

    // The report tool's rules for a frame that would look wrong: one the window server never
    // showed, one that landed a refresh late, and one whose step is out of line with its
    // neighbours', which is what a beat between the mouse rate and the refresh rate looks like.
    std::vector<flagged_frame> flag_frames(double refresh_ms) const {
        std::vector<flagged_frame> flagged;
        for (size_t i = 0; i < presented_.size(); ++i) {
            const presented_frame& frame = presented_[i];
            std::string flags;
            auto add = [&flags](const std::string& flag) {
                flags += flags.empty() ? "" : "; ";
                flags += flag;
            };
            if (frame.time <= 0.0) {
                add("never displayed");
            } else if (frame.has_step) {
                double late = frame.interval_ms / refresh_ms;
                if (late > kLateRefreshes && late < 12.0 && std::abs(frame.step) >= kVisibleStep) {
                    char text[64];
                    std::snprintf(text, sizeof text, "late %.0f refresh%s", late - 1.0,
                                  late < 2.5 ? "" : "es");
                    add(text);
                }
                if (i > 0 && i + 1 < presented_.size()) {
                    const presented_frame& before = presented_[i - 1];
                    const presented_frame& after = presented_[i + 1];
                    if (before.time > 0.0 && after.time > 0.0 && before.run == frame.run &&
                        after.run == frame.run && before.has_step && after.has_step) {
                        double reference = (before.step + after.step) * 0.5;
                        if (std::abs(reference) >= kVisibleStep) {
                            double deviation = frame.step - reference;
                            if (std::abs(deviation) > kStepDeviation * std::abs(reference)) {
                                const char* what =
                                    frame.step * reference < 0.0
                                        ? "reverse"
                                        : (std::abs(frame.step) < std::abs(reference) ? "short"
                                                                                      : "long");
                                char text[96];
                                std::snprintf(text, sizeof text, "%s step (%.1f vs %.1f)", what,
                                              frame.step, reference);
                                add(text);
                            }
                        }
                    }
                }
            }
            if (!flags.empty()) {
                flagged.push_back(flagged_frame{i, std::move(flags)});
            }
        }
        return flagged;
    }

    // Each displayed step against the mean of its two neighbours, for frames whose neighbours were
    // displayed in the same run. Unlike presented_step_error this does not charge the pipeline's
    // latency against the hand's changes of speed, so it is the number to read for smoothness.
    std::vector<double> step_deviations() const {
        std::vector<double> deviations;
        for (size_t i = 1; i + 1 < presented_.size(); ++i) {
            const presented_frame& before = presented_[i - 1];
            const presented_frame& frame = presented_[i];
            const presented_frame& after = presented_[i + 1];
            if (frame.time <= 0.0 || before.time <= 0.0 || after.time <= 0.0 || !frame.has_step ||
                !before.has_step || !after.has_step || before.run != frame.run ||
                after.run != frame.run) {
                continue;
            }
            double reference = (before.step + after.step) * 0.5;
            if (std::abs(reference) >= kVisibleStep) {
                deviations.push_back(std::abs(frame.step - reference));
            }
        }
        return deviations;
    }

    void report() {
        if (reported_) {
            return;
        }
        reported_ = true;
        px_set_frame_presented_callback(window_->px_window(), {});

        const char* mode = options_.replay_path ? "replay" : "record";
        const char* trace = options_.replay_path   ? options_.replay_path
                            : options_.record_path ? options_.record_path
                                                   : "-";
        vec2 size = window_->size();
        rect track = scrollbar_track();
        rect thumb = scrollbar_thumb();
        double travel = track.h - thumb.h;
        double refresh_ms = presentation_intervals_ms_.empty()
                                ? kAssumedRefreshMs
                                : quantile(presentation_intervals_ms_, 0.5);
        double duration =
            last_event_time_ > first_press_time_ ? last_event_time_ - first_press_time_ : 0.0;
        size_t late_frames = 0;
        for (double interval : presentation_intervals_ms_) {
            if (interval > kLateRefreshes * refresh_ms) {
                ++late_frames;
            }
        }
        std::vector<flagged_frame> flagged = flag_frames(refresh_ms);
        size_t irregular_steps = 0;
        for (const flagged_frame& frame : flagged) {
            if (frame.flags.find("step") != std::string::npos) {
                ++irregular_steps;
            }
        }

        std::printf("benchmark=scrollbar_drag mode=%s path=%s backend=%s trace=%s "
                    "viewport=%.0fx%.0f font_size=%g lines=%zu line_height=%.2f thumb=%.1fpt "
                    "travel=%.1fpt content_per_pointer_pt=%.3f\n",
                    mode, options_.per_event ? "per-event" : "tick",
                    backend_ ? backend_ : "unknown", trace, size.x, size.y, body_font_size_,
                    kDocumentLineCount, line_height_, thumb.h, travel,
                    travel > 0.0 ? maximum_scroll_offset() / travel : 0.0);
        std::printf("drags=%zu drag_events=%zu releases=%zu duration=%.3fs event_rate=%.1fHz\n",
                    drag_count_, drag_events_, releases_, duration,
                    event_intervals_ms_.empty() ? 0.0
                                                : 1000.0 / quantile(event_intervals_ms_, 0.5));
        print_distribution("event_interval", "ms", event_intervals_ms_);
        print_distribution("event_lag", "ms", event_lags_ms_);
        print_distribution("events_per_frame", "", events_per_frame_);
        print_distribution("paint", "ms", paint_ms_);
        print_distribution("paint_interval", "ms", paint_intervals_ms_);
        print_distribution("tick_delay", "ms", tick_delays_ms_);
        print_distribution("presentation_interval", "ms", presentation_intervals_ms_);
        print_distribution("paint_to_present", "ms", paint_to_present_ms_);
        print_distribution("input_to_present", "ms", input_to_present_ms_);
        print_distribution("presented_step", "pt", presented_steps_);
        print_distribution("step_deviation", "pt", step_deviations());
        print_distribution("presented_step_error", "pt", presented_step_errors_);
        print_distribution("presented_position_error", "pt", position_errors_);
        std::printf("summary refresh=%.3fms paints=%zu folded_events=%zu presented=%zu "
                    "never_displayed=%zu superseded=%zu late_frames=%zu irregular_steps=%zu "
                    "flagged_frames=%zu\n",
                    refresh_ms, drag_paints_, drag_events_ - std::min(drag_events_, drag_paints_),
                    presented_.size() - never_displayed_, never_displayed_,
                    awaiting_presentation_.size(), late_frames, irregular_steps, flagged.size());
        if (drag_paints_ > 0 && presented_.empty() && awaiting_presentation_.empty()) {
            std::printf("no presentation telemetry: this backend reports no frame ids\n");
        }
        if (options_.replay_path) {
            const Trace& expected = replay_;
            if (expected.window_width > 0.0 &&
                (expected.window_width != size.x || expected.window_height != size.y)) {
                std::printf("WARNING: the trace was recorded in a %.0fx%.0f window; this one is "
                            "%.0fx%.0f, so the thumb may not be where the press lands\n",
                            expected.window_width, expected.window_height, size.x, size.y);
            }
            size_t expected_drags = 0;
            for (const Sample& sample : expected.samples) {
                if (sample.kind == Kind::drag) {
                    ++expected_drags;
                }
            }
            if (drag_events_ > expected_drags) {
                std::printf("WARNING: %zu drag events arrived but the trace has %zu: other input "
                            "got in during the replay\n",
                            drag_events_, expected_drags);
            }
        }
        if (!flagged.empty()) {
            std::printf("flagged frames (times from the first press):\n");
            for (size_t i = 0; i < flagged.size() && i < kMaximumListedFrames; ++i) {
                const presented_frame& frame = presented_[flagged[i].index];
                if (frame.time <= 0.0) {
                    std::printf("  frame=%llu never displayed offset=%.2f\n",
                                static_cast<unsigned long long>(frame.frame_id), frame.offset);
                } else {
                    std::printf("  t=%.3fs frame=%llu interval=%.3fms step=%.2fpt  <-- %s\n",
                                frame.time - first_press_time_,
                                static_cast<unsigned long long>(frame.frame_id), frame.interval_ms,
                                frame.step, flagged[i].flags.c_str());
                }
            }
            if (flagged.size() > kMaximumListedFrames) {
                std::printf("  and %zu more\n", flagged.size() - kMaximumListedFrames);
            }
        }
        std::fflush(stdout);
    }

    // ── the scene ───────────────────────────────────────────────────────────────────────────────

    void draw_scene(px_render_context* context, rect bounds) {
        context->begin_rect_batch();
        context->draw_rect(bounds, kWindowBackground);
        context->draw_rect(rect{0.0, 0.0, kSidebarWidth, bounds.h}, kSidebarBackground);
        context->draw_rect(
            rect{0.0, sidebar_row_top(kSelectedSidebarLine), kSidebarWidth, sidebar_row_height()},
            kSidebarSelection);
        context->draw_rect(rect{kSidebarWidth, 0.0, bounds.w - kSidebarWidth, kTabStripHeight},
                           kTabStripBackground);
        context->end_rect_batch();

        context->begin_text_batch();
        for (size_t i = 0; i < sidebar_layouts_.size(); ++i) {
            fcolor color = i == 0 ? kSidebarTitleColor : kSidebarTextColor;
            px_font_t* font = i == 0 ? sidebar_title_font_ : sidebar_font_;
            size_t indent_level = kSidebarLines[i].indent_level;
            double indent = indent_level == 0
                                ? 0.0
                                : kSidebarIndentOffset + indent_level * kSidebarIndentWidth;
            double x = kSidebarLeftPadding + indent;
            draw_retained_text(context, font, vec2{x, sidebar_text_baseline(i)}, color,
                               &sidebar_layouts_[i]);
        }
        context->end_text_batch();

        context->push_state(false);
        context->restrict_clip_rect(rect{kSidebarWidth, kTabStripHeight, bounds.w - kSidebarWidth,
                                         content_bottom() - kTabStripHeight});
        double scroll_offset = scroll_.offset();
        int first_line = std::max(
            0, static_cast<int>(std::floor((scroll_offset - kTextTop) / line_height_)) - 1);
        int last_line = std::min(
            static_cast<int>(kDocumentLineCount),
            static_cast<int>(std::ceil((scroll_offset + bounds.h - kTextTop) / line_height_)) + 1);
        context->begin_rect_batch();
        for (int line = first_line; line < last_line; ++line) {
            size_t i = static_cast<size_t>(line);
            if (!has_non_ascii(kSourceLines[i % kSourceLines.size()])) {
                continue;
            }
            double baseline = kTextTop + i * line_height_ - scroll_offset;
            context->draw_rect(rect{kSidebarWidth + gutter_width_, baseline - body_metrics_.ascent,
                                    bounds.w - kSidebarWidth - gutter_width_, line_height_},
                               kLineBand);
        }
        context->end_rect_batch();
        context->begin_text_batch();
        for (int line = first_line; line < last_line; ++line) {
            size_t i = static_cast<size_t>(line);
            double y = kTextTop + i * line_height_ - scroll_offset;
            double number_x = kSidebarWidth + line_number_right_ - line_number_layouts_[i].advance;
            draw_layout(context, body_font_, vec2{number_x, y}, kLineNumberColor,
                        &line_number_layouts_[i]);
            draw_highlighted_line(context, body_font_,
                                  vec2{kSidebarWidth + gutter_width_ + kMargin, y},
                                  &highlighted_lines_[i % kSourceLines.size()]);
        }
        context->end_text_batch();
        context->pop_state();

        rect track = scrollbar_track();
        rect thumb = scrollbar_thumb();
        context->begin_rect_batch();
        context->draw_rect(track, kScrollbarTrack);
        context->draw_rect(thumb, dragging_scrollbar_ ? kScrollbarThumbDragging : kScrollbarThumb);
        context->end_rect_batch();

        draw_find_panel(context);
        draw_status_bar(context);
    }

    FindPanelLayout find_panel_layout() const {
        vec2 size = window_->size();
        FindPanelLayout layout;
        layout.panel =
            rect{0.0, size.y - kStatusBarHeight - kFindPanelHeight, size.x, kFindPanelHeight};
        double y = layout.panel.y + (kFindPanelHeight - kFindItemHeight) * 0.5;

        double x = kFindPanelPadding;
        layout.close = rect{x, y, kFindItemHeight, kFindItemHeight};
        x += kFindItemHeight + kFindItemGap;
        for (rect& toggle : layout.toggles) {
            toggle = rect{x, y, kFindToggleWidth, kFindItemHeight};
            x += kFindToggleWidth + kFindItemGap;
        }

        double right = size.x - kFindPanelPadding;
        for (size_t i = layout.buttons.size(); i-- > 0;) {
            double w = find_button_layouts_[i].advance + 2.0 * kFindButtonPadding;
            layout.buttons[i] = rect{right - w, y, w, kFindItemHeight};
            right -= w + kFindItemGap;
        }

        layout.input = rect{x, y, std::max(0.0, right - x), kFindItemHeight};
        return layout;
    }

    static void draw_centered(px_render_context* context,
                              px_font_t* font,
                              const px_font_metrics& metrics,
                              rect box,
                              fcolor color,
                              PreparedText* text) {
        double x = box.x + (box.w - text->advance) * 0.5;
        double baseline = box.y + (box.h - metrics.line_height) * 0.5 + metrics.ascent;
        draw_layout(context, font, vec2{x, baseline}, color, text);
    }

    void draw_find_panel(px_render_context* context) {
        FindPanelLayout layout = find_panel_layout();

        context->begin_rect_batch();
        context->draw_rect(layout.panel, kFindPanelBackground);
        for (const rect& toggle : layout.toggles) {
            context->draw_rect(toggle, kFindButtonBackground);
        }
        context->draw_rect(layout.input, kFindInputBackground);
        for (const rect& button : layout.buttons) {
            context->draw_rect(button, kFindButtonBackground);
        }
        double caret_x = layout.input.x + kFindTextInset;
        context->draw_rect(rect{caret_x, layout.input.y + 5.0, 1.0, kFindItemHeight - 10.0},
                           kFindLabelColor);
        context->end_rect_batch();

        context->begin_text_batch();
        draw_centered(context, sidebar_font_, sidebar_metrics_, layout.close, kFindMutedColor,
                      &find_close_layout_);
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.toggles[i],
                          kFindLabelColor, &find_toggle_layouts_[i]);
        }
        for (size_t i = 0; i < layout.buttons.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.buttons[i],
                          kFindLabelColor, &find_button_layouts_[i]);
        }
        context->end_text_batch();
    }

    void draw_status_bar(px_render_context* context) {
        vec2 size = window_->size();
        rect bar{0.0, size.y - kStatusBarHeight, size.x, kStatusBarHeight};
        context->begin_rect_batch();
        context->draw_rect(bar, kStatusBarBackground);
        context->end_rect_batch();

        double baseline =
            bar.y + (bar.h - status_metrics_.line_height) * 0.5 + status_metrics_.ascent;
        context->begin_text_batch();
        draw_layout(context, status_font_, vec2{kStatusBarPadding, baseline}, kStatusBarTextColor,
                    &status_layout_);
        context->end_text_batch();
    }

    // (Re)builds everything shaped in the body font, which the gutter shares.
    void set_body_font_size(float size) {
        size = std::clamp(size, kMinimumFontSize, kMaximumFontSize);
        body_font_size_ = size;
        body_font_ = px_create_font(kBodyFontFamily, size);
        body_metrics_ = px_font_get_metrics(body_font_);
        line_height_ = body_metrics_.line_height;
        grapheme_shaper* body_shaper = grapheme_shaper::instance(body_font_);
        double zero_advance = prepare_retained_text(body_shaper, "0").advance;
        line_number_right_ = kGutterMiniDiffWidth +
                             std::ceil(kGutterLeftPadding + kLineNumberDigits * zero_advance);
        gutter_width_ = line_number_right_ + kGutterNumberPadding + kGutterMiniDiffWidth +
                        kGutterMargin + kGutterFoldButtonWidth;
        for (size_t i = 0; i < kSourceLines.size(); ++i) {
            highlighted_lines_[i] = highlight_line(body_shaper, kSourceLines[i]);
        }
        line_number_layouts_.resize(kDocumentLineCount);
        for (size_t i = 0; i < kDocumentLineCount; ++i) {
            line_number_layouts_[i] = prepare_retained_text(body_shaper, std::to_string(i + 1));
        }
    }

    double sidebar_row_height() const {
        return kSidebarRowTopPadding + sidebar_title_metrics_.line_height +
               kSidebarRowBottomPadding;
    }

    double sidebar_row_top(size_t index) const {
        return kSidebarTopPadding + index * sidebar_row_height();
    }

    double sidebar_text_baseline(size_t index) const {
        const px_font_metrics& metrics = index == 0 ? sidebar_title_metrics_ : sidebar_metrics_;
        return sidebar_row_top(index) + kSidebarRowTopPadding +
               (sidebar_title_metrics_.line_height - metrics.line_height) * 0.5 + metrics.ascent;
    }

    window* window_ = nullptr;
    px_window_event_handler* funnel_ = nullptr;
    Options options_;
    px_font_t* body_font_ = nullptr;
    float body_font_size_ = 0.0f;
    px_font_t* sidebar_title_font_ = nullptr;
    px_font_t* sidebar_font_ = nullptr;
    px_font_t* status_font_ = nullptr;
    double line_height_ = 0.0;
    double line_number_right_ = 0.0;
    double gutter_width_ = 0.0;
    px_font_metrics body_metrics_;
    px_font_metrics sidebar_title_metrics_;
    px_font_metrics sidebar_metrics_;
    px_font_metrics status_metrics_;
    smooth_scroll scroll_;
    bool dragging_scrollbar_ = false;
    double scrollbar_drag_offset_ = 0.0;
    PreparedText find_close_layout_;
    PreparedText status_layout_;
    std::array<PreparedText, kFindToggleLabels.size()> find_toggle_layouts_;
    std::array<PreparedText, kFindButtonLabels.size()> find_button_layouts_;
    std::array<HighlightedLine, kSourceLines.size()> highlighted_lines_;
    std::vector<PreparedText> line_number_layouts_;
    std::array<PreparedText, kSidebarLines.size()> sidebar_layouts_;

    // Measurement.
    const char* backend_ = nullptr;
    bool pressed_in_track_ = false;
    bool reported_ = false;
    bool finishing_ = false;
    bool recording_written_ = false;
    bool injecting_ = false;
    bool have_previous_presented_ = false;
    int run_ = 0;
    int previous_paint_run_ = 0;
    size_t drag_count_ = 0;
    size_t drag_events_ = 0;
    size_t pending_drag_events_ = 0;
    size_t drag_paints_ = 0;
    size_t releases_ = 0;
    size_t never_displayed_ = 0;
    double first_press_time_ = 0.0;
    double last_event_time_ = 0.0;
    double last_painted_offset_ = 0.0;
    double previous_paint_time_ = 0.0;
    presented_frame previous_presented_;
    Trace recording_;
    Trace replay_;
    std::vector<trajectory_point> trajectory_;
    std::unordered_map<uint64_t, painted_frame> awaiting_presentation_;
    std::vector<presented_frame> presented_;
    std::vector<double> event_intervals_ms_;
    std::vector<double> event_lags_ms_;
    std::vector<double> events_per_frame_;
    std::vector<double> paint_ms_;
    std::vector<double> paint_intervals_ms_;
    std::vector<double> tick_delays_ms_;
    std::vector<double> presentation_intervals_ms_;
    std::vector<double> paint_to_present_ms_;
    std::vector<double> input_to_present_ms_;
    std::vector<double> presented_steps_;
    std::vector<double> presented_step_errors_;
    std::vector<double> position_errors_;
};

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, &options)) {
        usage(argv[0]);
        return 2;
    }

    Trace trace;
    if (options.replay_path) {
        std::string error;
        if (!read_trace(options.replay_path, &trace, &error)) {
            std::fprintf(stderr, "scrollbar_benchmark: %s\n", error.c_str());
            return 3;
        }
    }
    if (options.record_path && std::ifstream(options.record_path)) {
        std::fprintf(stderr, "scrollbar_benchmark: refusing to overwrite %s\n",
                     options.record_path);
        return 2;
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("scrollbar-benchmark", "com.example.scrollbar-benchmark", argc, argv, 0);

    window_impl window(kDefaultWindowWidth, kDefaultWindowHeight, "scrollbar benchmark",
                       kWindowBackground);
    window_basic_aspect basic(&window);
    window.add_window_aspect(&basic);

    // Maximized like the editor. A trace recorded that way replays that way; any other size (the
    // trace's, or --size) is set explicitly and anchored near the top-left so it stays on screen.
    window.set_maximized(true);
    double width = options.width;
    double height = options.height;
    if (width == 0.0 && trace.window_width > 0.0) {
        width = trace.window_width;
        height = trace.window_height;
    }
    vec2 maximized = window.size();
    if (width > 0.0 && (width != maximized.x || height != maximized.y)) {
        px_set_window_size(window.px_window(), width, height);
        px_set_window_position(window.px_window(), vec2{20.0, 40.0});
    }

    float font_size = options.font_size > 0.0f ? options.font_size
                      : trace.font_size > 0.0f ? trace.font_size
                                               : kMainFontSize;
    px_font_t* sidebar_title_font = px_create_font("system", kSidebarTitleFontSize, PX_FONT_BOLD);
    px_font_t* sidebar_font = px_create_font("system", kSidebarFontSize);
    ScrollbarBenchmark root(&window, &window, options, sidebar_title_font, sidebar_font,
                            font_size);
    window.set_root_control(&root);
    px_set_application_event_handler(&root);
    px_set_frame_presented_callback(window.px_window(),
                                    [&root](uint64_t frame_id, double presented_time) {
                                        root.frame_presented(frame_id, presented_time);
                                    });
    window.show();
    if (options.replay_path) {
        px_set_timeout(
            [&root, trace = std::move(trace)]() mutable { root.start_replay(std::move(trace)); },
            kReplayLeadMs);
    } else {
        std::printf("drag the scrollbar thumb; Escape reports and quits\n");
    }

    px_run_event_loop();
    return 0;
}
