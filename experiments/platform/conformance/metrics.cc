#include "experiments/platform/px/grapheme_shaper.h"
#include "experiments/platform/px/px.h"

#include "base/unicode/unicode.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct PointMetric {
    size_t point = 0;
    size_t row = 0;
    size_t column = 0;
    size_t utf8_column = 0;
    size_t utf16_column = 0;
    double x = 0.0;
    double y = 0.0;
};

struct LineOrigin {
    size_t row = 0;
    double y = 0.0;
};

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void write_json_string(std::ostream& output, std::string_view value) {
    output.put('"');
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\b':
            output << "\\b";
            break;
        case '\f':
            output << "\\f";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (byte < 0x20) {
                constexpr char kHex[] = "0123456789abcdef";
                output << "\\u00" << kHex[byte >> 4] << kHex[byte & 0xf];
            } else {
                output.put(static_cast<char>(byte));
            }
        }
    }
    output.put('"');
}

const char* platform_name() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "osx";
#else
    return "linux";
#endif
}

const char* architecture_name() {
#if defined(_M_ARM64) || defined(__aarch64__)
    return "arm64";
#elif defined(_M_X64) || defined(__x86_64__)
    return "x64";
#elif defined(_M_IX86) || defined(__i386__)
    return "x86";
#else
    return "unknown";
#endif
}

bool parse_size(const char* text, float* result) {
    char* end = nullptr;
    errno = 0;
    const float value = std::strtof(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !std::isfinite(value) || value <= 0.0f) {
        return false;
    }
    *result = value;
    return true;
}

bool collect_points(std::string_view text,
                    grapheme_shaper* shaper,
                    double line_height,
                    std::vector<PointMetric>* points,
                    std::vector<LineOrigin>* line_origins,
                    double* maximum_width) {
    size_t byte_offset = 0;
    size_t line_start = 0;
    size_t point = 0;
    size_t row = 0;
    size_t column = 0;
    size_t utf16_column = 0;

    line_origins->push_back({.row = row, .y = 0.0});
    while (true) {
        const std::string_view prefix = text.substr(line_start, byte_offset - line_start);
        const double line_x = shaper->measure_string(prefix);
        points->push_back({
            .point = point,
            .row = row,
            .column = column,
            .utf8_column = byte_offset - line_start,
            .utf16_column = utf16_column,
            .x = line_x,
            .y = static_cast<double>(row) * line_height,
        });

        if (byte_offset == text.size()) {
            *maximum_width = std::max(*maximum_width, line_x);
            return true;
        }

        size_t next_offset = byte_offset;
        const base::Unichar codepoint = base::next_utf8(text, next_offset);
        if (codepoint < 0 || next_offset <= byte_offset) {
            return false;
        }

        ++point;
        byte_offset = next_offset;
        if (codepoint == '\n') {
            *maximum_width = std::max(*maximum_width, line_x + shaper->measure_glyph(U'\n'));
            ++row;
            column = 0;
            utf16_column = 0;
            line_start = byte_offset;
            line_origins->push_back({
                .row = row,
                .y = static_cast<double>(row) * line_height,
            });
        } else {
            ++column;
            utf16_column += codepoint > 0xffff ? 2 : 1;
        }
    }
}

bool write_metrics(const std::filesystem::path& output_path,
                   std::string_view text_name,
                   std::string_view face,
                   float size,
                   const px_font_metrics& font_metrics,
                   float em_width,
                   const std::vector<LineOrigin>& line_origins,
                   const std::vector<PointMetric>& points,
                   double maximum_width) {
    std::error_code error;
    if (!output_path.parent_path().empty()) {
        std::filesystem::create_directories(output_path.parent_path(), error);
        if (error) {
            std::println(stderr, "cannot create {}: {}", output_path.parent_path().string(),
                         error.message());
            return false;
        }
    }

    std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        std::println(stderr, "cannot write {}", output_path.string());
        return false;
    }
    output.imbue(std::locale::classic());
    output << std::setprecision(std::numeric_limits<double>::max_digits10);

    output << "{\n  \"schema_version\": 1,\n  \"engine\": \"simple-text\",\n";
    output << "  \"platform\": ";
    write_json_string(output, platform_name());
    output << ",\n  \"arch\": ";
    write_json_string(output, architecture_name());
    output << ",\n  \"text_name\": ";
    write_json_string(output, text_name);
    output << ",\n  \"face\": ";
    write_json_string(output, face);
    output << ",\n  \"size\": " << size << ",\n  \"font_options\": [],\n";
    output << "  \"position_model\": \"grapheme_shaper_logical_prefix\",\n";
    output << "  \"line_height\": " << font_metrics.line_height << ",\n";
    output << "  \"em_width\": " << em_width << ",\n";
    output << "  \"ascent\": " << font_metrics.ascent << ",\n";
    output << "  \"descent\": " << font_metrics.descent << ",\n";
    output << "  \"leading\": " << font_metrics.leading << ",\n";
    output << "  \"layout_extent\": [" << maximum_width << ", "
           << static_cast<double>(line_origins.size()) * font_metrics.line_height + 1.0 << "],\n";

    output << "  \"line_origins\": [\n";
    for (size_t index = 0; index < line_origins.size(); ++index) {
        const LineOrigin& origin = line_origins[index];
        output << "    {\"row\": " << origin.row << ", \"x\": 0, \"y\": " << origin.y << "}";
        output << (index + 1 == line_origins.size() ? "\n" : ",\n");
    }
    output << "  ],\n  \"points\": [\n";
    for (size_t index = 0; index < points.size(); ++index) {
        const PointMetric& metric = points[index];
        output << "    {\"point\": " << metric.point << ", \"row\": " << metric.row
               << ", \"column\": " << metric.column << ", \"utf8_column\": " << metric.utf8_column
               << ", \"utf16_column\": " << metric.utf16_column << ", \"x\": " << metric.x
               << ", \"y\": " << metric.y << ", \"line_x\": " << metric.x
               << ", \"layout_roundtrip_point\": null}";
        output << (index + 1 == points.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    return output.good();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::println(stderr, "usage: {} TEXT_PATH FACE SIZE OUTPUT_PATH", argv[0]);
        return 2;
    }

    float size = 0.0f;
    if (!parse_size(argv[3], &size)) {
        std::println(stderr, "invalid font size: {}", argv[3]);
        return 2;
    }

    const std::filesystem::path text_path = argv[1];
    std::ifstream input_check(text_path, std::ios::binary);
    if (!input_check) {
        std::println(stderr, "cannot read {}", text_path.string());
        return 1;
    }
    input_check.close();
    const std::string text = read_file(text_path);

    px_font_t* font = px_create_font(argv[2], size);
    if (!font) {
        std::println(stderr, "cannot create font {} at {}", argv[2], size);
        return 1;
    }
    grapheme_shaper* shaper = grapheme_shaper::instance(font);
    const px_font_metrics font_metrics = px_font_get_metrics(font);

    std::vector<PointMetric> points;
    std::vector<LineOrigin> line_origins;
    double maximum_width = 0.0;
    if (!collect_points(text, shaper, font_metrics.line_height, &points, &line_origins,
                        &maximum_width)) {
        std::println(stderr, "{} is not valid UTF-8", text_path.string());
        return 1;
    }

    return write_metrics(argv[4], text_path.filename().string(), argv[2], size, font_metrics,
                         px_font_em_width(font), line_origins, points, maximum_width)
               ? 0
               : 1;
}
