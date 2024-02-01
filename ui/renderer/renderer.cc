#include "renderer.h"
#include "util/file_util.h"
#include "util/log_util.h"
#include "util/opengl_error_util.h"
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

struct InstanceData {
    // Grid coordinates.
    uint16_t col;
    uint16_t row;
    // Glyph properties.
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    // UV mapping.
    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
    // Color, packed with colored flag.
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t colored;
    // Total font advance.
    float total_advance;
    // Background color.
    uint8_t bg_r;
    uint8_t bg_g;
    uint8_t bg_b;
    uint8_t bg_a;
    // Glyph advance.
    float advance;
};

extern "C" TSLanguage* tree_sitter_json();

Renderer::Renderer(float width, float height, std::string main_font_name,
                   std::string emoji_font_name, int font_size) {
    rasterizer = new Rasterizer(main_font_name, emoji_font_name, font_size);
    atlas_renderer = new AtlasRenderer(width, height);
    cursor_renderer = new CursorRenderer(width, height);

    this->linkShaders();
    this->resize(width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    // uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    // this->treeSitterExperiment();
    // uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    // uint64_t microseconds = (end - start) / 1e3;
    // float fps = 1000000.0 / microseconds;
    // LogDefault("Renderer", "Tree-sitter: %ld µs (%f fps)", microseconds, fps);

    parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_json());

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    const char* query_code = ReadFile(ResourcePath("highlights.scm"));
    query = ts_query_new(tree_sitter_json(), query_code, strlen(query_code), &error_offset,
                         &error_type);

    if (error_type != TSQueryErrorNone) {
        LogError("Renderer", "Error creating new TSQuery. error_offset: %d, error type: %d",
                 error_offset, error_type);
    }

    std::vector<std::string> capture_names;
    uint32_t capture_count = ts_query_capture_count(query);
    for (int i = 0; i < capture_count; i++) {
        uint32_t length;
        const char* capture_name = ts_query_capture_name_for_id(query, i, &length);
        capture_names.push_back(capture_name);
    }

    // Font experiments.
    // NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Source Code Pro"};
    // CTFontDescriptorRef descriptor =
    //     CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
    // CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
    // CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
    // CFArrayRef fontDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor,
    // NULL);

    // for (int i = 0; i < CFArrayGetCount(fontDescriptors); i++) {
    //     CTFontDescriptorRef descriptor =
    //         (CTFontDescriptorRef)CFArrayGetValueAtIndex(fontDescriptors, i);
    //     CFStringRef familyName =
    //         (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
    //     CFStringRef style =
    //         (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);

    //     if (CFEqual(style, CFSTR("Italic"))) {
    //         LogDefault("Renderer", "%@ %@", familyName, style);
    //         CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, font_size, nullptr);
    //         this->mainFont = tempFont;
    //     }
    // }
    // End of font experiments.

    Metrics metrics = rasterizer->metrics();
    float cell_width = std::floor(metrics.average_advance + 1);
    float cell_height = std::floor(metrics.line_height + 2);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "cell_dim"), cell_width, cell_height);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_instance);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData) * BATCH_MAX, nullptr, GL_STATIC_DRAW);

    size_t size = 0;

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(0, 1);
    size += 2 * sizeof(uint16_t);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_INT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(1, 1);
    size += 4 * sizeof(int32_t);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(2, 1);
    size += 4 * sizeof(float);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(3, 1);
    size += 4 * sizeof(uint8_t);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(4, 1);
    size += sizeof(float);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(5, 1);
    size += 4 * sizeof(uint8_t);

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(6, 1);
    size += sizeof(float);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

struct buffer_t {
    const char* buf;
    size_t len;
};

const char* read(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    if (byte_index >= ((buffer_t*)payload)->len) {
        *bytes_read = 0;
        return (char*)"";
    } else {
        *bytes_read = 1;
        return (char*)(((buffer_t*)payload)->buf) + byte_index;
    }
}

void Renderer::parseBuffer(Buffer& buffer) {
    const char* source_code = buffer.toCString();
    buffer_t buf = {source_code, strlen(source_code)};
    TSInput input = {&buf, read, TSInputEncodingUTF8};
    tree = ts_parser_parse(parser, tree, input);

    this->highlight();
}

void Renderer::editBuffer(Buffer& buffer) {
    const char* source_code = buffer.toCString();

    TSInputEdit edit = {
        static_cast<uint32_t>(drag_cursor_row),
        static_cast<uint32_t>(strlen(source_code) - 1),
        static_cast<uint32_t>(strlen(source_code)),
        // These are unused!
        {0, 0},
        {0, 0},
        {0, 0},
    };
    ts_tree_edit(tree, &edit);

    buffer_t buf = {source_code, strlen(source_code)};
    TSInput input = {&buf, read, TSInputEncodingUTF8};
    tree = ts_parser_parse(parser, tree, input);

    this->highlight();
}

void Renderer::highlight() {
    TSNode root_node = ts_tree_root_node(tree);
    TSQueryCursor* query_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(query_cursor, query, root_node);

    const void* prev_id = 0;
    uint32_t prev_start = -1;
    uint32_t prev_end = -1;

    // TODO: Profile this code and optimize it to be as fast as Tree-sitter's CLI.
    TSQueryMatch match;
    uint32_t capture_index;

    highlight_ranges.clear();
    highlight_colors.clear();

    while (ts_query_cursor_next_capture(query_cursor, &match, &capture_index)) {
        TSQueryCapture capture = match.captures[capture_index];
        TSNode node = capture.node;
        uint32_t start_byte = ts_node_start_byte(node);
        uint32_t end_byte = ts_node_end_byte(node);

        if (start_byte != prev_start && end_byte != prev_end && node.id != prev_id) {
            // DEV: This significantly slows down highlighting. Only enable when debugging.
            // LogDefault("Renderer", "%d, [%d, %d], %s", node.id, start_byte, end_byte,
            //            capture_names[capture.index]);

            highlight_ranges.push_back({start_byte, end_byte});
            if (capture.index == 0) {
                highlight_colors.push_back(PURPLE);
            } else if (capture.index == 1) {
                highlight_colors.push_back(GREEN);
            } else if (capture.index == 2) {
                highlight_colors.push_back(YELLOW);
            } else if (capture.index == 3) {
                highlight_colors.push_back(RED);
            } else if (capture.index == 5) {
                highlight_colors.push_back(GREY2);
            } else {
                highlight_colors.push_back(BLACK);
            }
        }

        prev_id = node.id;
        prev_start = start_byte;
        prev_end = end_byte;
    }
}

float Renderer::getGlyphAdvance(const char* utf8_str) {
    uint32_t unicode_scalar = utf8_str[0];

    LogDefault("Renderer", "getGlyphAdvance for %s: %d", utf8_str, unicode_scalar);

    if (!glyph_cache.count(unicode_scalar)) {
        this->loadGlyph(unicode_scalar, utf8_str);
    }

    AtlasGlyph glyph = glyph_cache[unicode_scalar];
    return glyph.advance;
}

std::pair<float, size_t> Renderer::closestBoundaryForX(const char* line, float x) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; line[offset] != '\0'; offset += ret) {
        ret = grapheme_decode_utf8(line + offset, SIZE_MAX, NULL);

        uint32_t unicode_scalar = 0;
        for (int i = 0; i < ret; i++) {
            uint8_t byte = (line + offset)[i];
            unicode_scalar |= byte << 8 * i;
        }

        if (!glyph_cache.count(unicode_scalar)) {
            this->loadGlyph(unicode_scalar, line + offset);
            LogDefault("Renderer", "new unicode_scalar: %d", unicode_scalar);
        }

        AtlasGlyph glyph = glyph_cache[unicode_scalar];

        float glyph_center = total_advance + glyph.advance / 2;
        if (glyph_center >= x) {
            return {total_advance, offset};
        }

        total_advance += std::round(glyph.advance);
    }
    return {total_advance, offset};
}

void Renderer::renderText(Buffer& buffer, float scroll_x, float scroll_y) {
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "scroll_offset"), scroll_x, scroll_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;
    int range_idx = 0;

    Metrics metrics = rasterizer->metrics();
    float cell_width = std::floor(metrics.average_advance + 1);
    float cell_height = std::floor(metrics.line_height + 2);

    int row_offset = scroll_y / -cell_height;
    if (row_offset < 0) row_offset = 0;

    size_t byte_offset = buffer.byteOfLine(row_offset);
    size_t size = std::min(static_cast<size_t>(row_offset + 60), buffer.lineCount());
    for (int row = row_offset; row < size; row++) {
        const char* line = buffer.data[row].c_str();
        size_t ret;
        float total_advance = 0;
        for (size_t offset = 0; line[offset] != '\0'; offset += ret, byte_offset += ret) {
            ret = grapheme_decode_utf8(line + offset, SIZE_MAX, NULL);

            Rgb text_color = BLACK;

            if (range_idx < highlight_ranges.size()) {
                while (byte_offset >= highlight_ranges[range_idx].second) {
                    range_idx++;
                }

                if (highlight_ranges[range_idx].first <= byte_offset &&
                    byte_offset < highlight_ranges[range_idx].second) {
                    text_color = highlight_colors[range_idx];
                }
            }

            uint32_t unicode_scalar = 0;
            for (int i = 0; i < ret; i++) {
                uint8_t byte = (line + offset)[i];
                unicode_scalar |= byte << 8 * i;
            }

            if (!glyph_cache.count(unicode_scalar)) {
                this->loadGlyph(unicode_scalar, line + offset);
                LogDefault("Renderer", "new unicode_scalar: %d", unicode_scalar);
            }

            AtlasGlyph glyph = glyph_cache[unicode_scalar];

            float glyph_center_x = total_advance + glyph.advance / 2;
            uint8_t bg_a = this->isGlyphInSelection(row, glyph_center_x) ? 255 : 0;

            instances.push_back(InstanceData{
                // Grid coordinates.
                0,
                static_cast<uint16_t>(row),
                // Glyph properties.
                glyph.left,
                glyph.top,
                glyph.width,
                glyph.height,
                // UV mapping.
                glyph.uv_left,
                glyph.uv_bot,
                glyph.uv_width,
                glyph.uv_height,
                // Color, packed with colored flag.
                text_color.r,
                text_color.g,
                text_color.b,
                glyph.colored,
                // Total font advance.
                total_advance,
                // Background color.
                YELLOW.r,
                YELLOW.g,
                YELLOW.b,
                bg_a,
                // Glyph advance.
                glyph.advance,
            });

            total_advance += std::round(glyph.advance);
        }
        byte_offset++;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glUniform1i(glGetUniformLocation(shader_program, "rendering_pass"), 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());
    glUniform1i(glGetUniformLocation(shader_program, "rendering_pass"), 1);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    atlas_renderer->draw(width - Atlas::ATLAS_SIZE, 500.0f, atlas.tex_id);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    atlas_renderer->draw(width - Atlas::ATLAS_SIZE, 500.0f, atlas.tex_id);
    glEnable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_BLEND);
    cursor_renderer->draw(scroll_x, scroll_y, drag_cursor_x, drag_cursor_row * cell_height,
                          cell_height);
    glEnable(GL_BLEND);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();
}

bool Renderer::isGlyphInSelection(int row, float glyph_center_x) {
    int start_row, end_row;
    float start_x, end_x;

    if (last_cursor_row == drag_cursor_row) {
        if (last_cursor_x <= drag_cursor_x) {
            start_row = last_cursor_row;
            end_row = drag_cursor_row;
            start_x = last_cursor_x;
            end_x = drag_cursor_x;
        } else {
            start_row = drag_cursor_row;
            end_row = last_cursor_row;
            start_x = drag_cursor_x;
            end_x = last_cursor_x;
        }
    } else if (last_cursor_row < drag_cursor_row) {
        start_row = last_cursor_row;
        end_row = drag_cursor_row;
        start_x = last_cursor_x;
        end_x = drag_cursor_x;
    } else {
        start_row = drag_cursor_row;
        end_row = last_cursor_row;
        start_x = drag_cursor_x;
        end_x = last_cursor_x;
    }

    if (start_row < row && row < end_row) {
        return true;
    }
    if (start_row == end_row) {
        if (row == start_row && start_x <= glyph_center_x && glyph_center_x <= end_x) {
            return true;
        }
    } else {
        if (row == start_row && start_x <= glyph_center_x) {
            return true;
        }
        if (row == end_row && glyph_center_x <= end_x) {
            return true;
        }
    }
    return false;
}

void Renderer::setCursorPositions(Buffer& buffer, float cursor_x, float cursor_y, float drag_x,
                                  float drag_y) {
    Metrics metrics = rasterizer->metrics();
    float cell_height = std::floor(metrics.line_height + 2);

    float x;
    size_t offset;

    last_cursor_row = cursor_y / cell_height;
    std::tie(x, offset) =
        this->closestBoundaryForX(buffer.data[last_cursor_row].c_str(), cursor_x);
    last_cursor_byte_offset = offset;
    last_cursor_x = x;

    drag_cursor_row = drag_y / cell_height;
    std::tie(x, offset) = this->closestBoundaryForX(buffer.data[drag_cursor_row].c_str(), drag_x);
    drag_cursor_byte_offset = offset;
    drag_cursor_x = x;
}

void Renderer::loadGlyph(uint32_t scalar, const char* utf8_str) {
    RasterizedGlyph glyph = rasterizer->rasterizeUTF8(utf8_str);
    AtlasGlyph atlas_glyph = atlas.insertGlyph(glyph);
    glyph_cache.insert({scalar, atlas_glyph});
}

void Renderer::resize(int new_width, int new_height) {
    width = new_width;
    height = new_height;

    glViewport(0, 0, width, height);
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);

    atlas_renderer->resize(width, height);
    cursor_renderer->resize(width, height);
}

void Renderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFile(ResourcePath("shaders/text_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("shaders/text_frag.glsl"));
    glShaderSource(vertex_shader, 1, &vert_source, nullptr);
    glShaderSource(fragment_shader, 1, &frag_source, nullptr);
    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo_instance);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader_program);
}
