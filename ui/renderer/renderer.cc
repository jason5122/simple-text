#include "base/rgb.h"
#include "renderer.h"
#include "util/file_util.h"
#include "util/log_util.h"
#include "util/opengl_error_util.h"
#include <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

struct InstanceData {
    uint32_t line;
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

extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_glsl();
extern "C" TSLanguage* tree_sitter_json();
extern "C" TSLanguage* tree_sitter_scheme();

Renderer::Renderer(float width, float height, std::string main_font_name,
                   std::string emoji_font_name, int font_size, float line_height)
    : line_height(line_height) {
    rasterizer = new Rasterizer(main_font_name, emoji_font_name, font_size);
    atlas_renderer = new AtlasRenderer(width, height);
    cursor_renderer = new CursorRenderer(width, height);
    highlighter.setLanguage("source.scheme");

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

    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "line_height"), line_height);

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
    glVertexAttribPointer(0, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(InstanceData), (void*)size);
    glVertexAttribDivisor(0, 1);
    size += sizeof(uint32_t);

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

void memchrsub(void* data, char c, char x, size_t len) {
    char* p = (char*)data;
    char* end = (char*)data + len;
    while ((p = (char*)memchr(p, c, (size_t)(end - p)))) {
        *p++ = x;
    }
}

const char* read(void* payload, uint32_t byte_index, TSPoint position, uint32_t* bytes_read) {
    Buffer* buffer = (Buffer*)payload;
    if (position.row >= buffer->lineCount()) {
        *bytes_read = 0;
        return "";
    }

    const size_t BUFSIZE = 256;
    static char buf[BUFSIZE];

    const char* line_str = buffer->data[position.row].c_str();
    size_t len = buffer->data[position.row].size();
    size_t tocopy = std::min(len - position.column, BUFSIZE);

    memcpy(buf, line_str + position.column, tocopy);
    memchrsub(buf, '\n', '\0', tocopy);  // Translate embedded \n to NUL.
    *bytes_read = (uint32_t)tocopy;
    if (tocopy < BUFSIZE) {
        // Add the final \n.
        // If it didn't fit, read() will be called again on the same line with the column advanced.
        buf[tocopy] = '\n';
        (*bytes_read)++;
    }
    return buf;
}

void Renderer::parseBuffer(Buffer& buffer) {
    TSInput input = {&buffer, read, TSInputEncodingUTF8};
    highlighter.parse(input);
    highlighter.getHighlights();
}

void Renderer::editBuffer(Buffer& buffer, size_t bytes) {
    size_t start_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset;
    size_t old_end_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset;
    size_t new_end_byte = buffer.byteOfLine(cursor_end_line) + cursor_end_col_offset + bytes;
    highlighter.edit(start_byte, old_end_byte, new_end_byte);
}

float Renderer::getGlyphAdvance(std::string utf8_str) {
    if (!glyph_cache.count(utf8_str)) {
        this->loadGlyph(utf8_str);
    }

    AtlasGlyph glyph = glyph_cache[utf8_str];
    return std::round(glyph.advance);
}

std::pair<float, size_t> Renderer::closestBoundaryForX(const char* line_str, float x) {
    size_t offset;
    size_t ret;
    float total_advance = 0;
    for (offset = 0; line_str[offset] != '\0'; offset += ret) {
        ret = grapheme_next_character_break_utf8(line_str + offset, SIZE_MAX);

        std::string utf8_str;
        for (size_t i = 0; i < ret; i++) {
            utf8_str += (line_str + offset)[i];
        }

        if (!glyph_cache.count(utf8_str)) {
            this->loadGlyph(utf8_str);
        }

        AtlasGlyph glyph = glyph_cache[utf8_str];

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

    size_t scroll_line = -scroll_y / line_height;

    size_t visible_lines = std::ceil(height / line_height);
    size_t byte_offset = buffer.byteOfLine(scroll_line);
    size_t size = std::min(static_cast<size_t>(scroll_line + visible_lines), buffer.lineCount());
    for (size_t line = scroll_line; line < size; line++) {
        const char* line_str = buffer.data[line].c_str();
        size_t ret;
        float total_advance = 0;
        for (size_t offset = 0; line_str[offset] != '\0'; offset += ret, byte_offset += ret) {
            ret = grapheme_next_character_break_utf8(line_str + offset, SIZE_MAX);

            Rgb text_color = BLACK;

            if (range_idx < highlighter.highlight_ranges.size()) {
                while (byte_offset >= highlighter.highlight_ranges[range_idx].second) {
                    range_idx++;
                }

                if (highlighter.highlight_ranges[range_idx].first <= byte_offset &&
                    byte_offset < highlighter.highlight_ranges[range_idx].second) {
                    text_color = highlighter.highlight_colors[range_idx];
                }
            }

            std::string utf8_str;
            for (size_t i = 0; i < ret; i++) {
                utf8_str += (line_str + offset)[i];
            }

            if (!glyph_cache.count(utf8_str)) {
                this->loadGlyph(utf8_str);
            }

            AtlasGlyph glyph = glyph_cache[utf8_str];

            float glyph_center_x = total_advance + glyph.advance / 2;
            uint8_t bg_a = this->isGlyphInSelection(line, glyph_center_x) ? 255 : 0;

            instances.push_back(InstanceData{
                static_cast<uint32_t>(line),
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
        longest_line_x = std::max(total_advance, longest_line_x);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);

    glUniform1i(glGetUniformLocation(shader_program, "rendering_pass"), 0);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());
    glUniform1i(glGetUniformLocation(shader_program, "rendering_pass"), 1);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // // Unbind.
    // glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    // glBindVertexArray(0);
    // glBindTexture(GL_TEXTURE_2D, 0);

    // atlas_renderer->draw(width - Atlas::ATLAS_SIZE, 500.0f, atlas.tex_id);
    // glDisable(GL_BLEND);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // atlas_renderer->draw(width - Atlas::ATLAS_SIZE, 500.0f, atlas.tex_id);
    // glEnable(GL_BLEND);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_BLEND);
    cursor_renderer->draw(scroll_x, scroll_y, cursor_end_x, cursor_end_line, line_height,
                          buffer.lineCount(), longest_line_x, visible_lines);
    glEnable(GL_BLEND);

    // // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // // https://learnopengl.com/In-Practice/Debugging
    // glPrintError();
}

bool Renderer::isGlyphInSelection(int row, float glyph_center_x) {
    int start_row, end_row;
    float start_x, end_x;

    if (cursor_start_line == cursor_end_line) {
        if (cursor_start_x <= cursor_end_x) {
            start_row = cursor_start_line;
            end_row = cursor_end_line;
            start_x = cursor_start_x;
            end_x = cursor_end_x;
        } else {
            start_row = cursor_end_line;
            end_row = cursor_start_line;
            start_x = cursor_end_x;
            end_x = cursor_start_x;
        }
    } else if (cursor_start_line < cursor_end_line) {
        start_row = cursor_start_line;
        end_row = cursor_end_line;
        start_x = cursor_start_x;
        end_x = cursor_end_x;
    } else {
        start_row = cursor_end_line;
        end_row = cursor_start_line;
        start_x = cursor_end_x;
        end_x = cursor_start_x;
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

void Renderer::setCursorPositions(Buffer& buffer, float scroll_x, float scroll_y, float cursor_x,
                                  float cursor_y, float drag_x, float drag_y) {
    float x;
    size_t offset;

    scroll_x = -scroll_x * 2;  // FIXME: Why do we need to multiply by content scale again?
    LogDefault("Renderer", "scroll_x = %f, width = %f", scroll_x, width);

    cursor_start_line = cursor_y / line_height;
    if (cursor_start_line > buffer.lineCount()) cursor_start_line = buffer.lineCount();
    std::tie(x, offset) =
        this->closestBoundaryForX(buffer.data[cursor_start_line].c_str(), cursor_x + scroll_x);
    cursor_start_col_offset = offset;
    cursor_start_x = x;

    cursor_end_line = drag_y / line_height;
    if (cursor_end_line > buffer.lineCount()) cursor_end_line = buffer.lineCount();
    std::tie(x, offset) =
        this->closestBoundaryForX(buffer.data[cursor_end_line].c_str(), drag_x + scroll_x);
    cursor_end_col_offset = offset;
    cursor_end_x = x;
}

void Renderer::loadGlyph(std::string utf8_str) {
    RasterizedGlyph glyph = rasterizer->rasterizeUTF8(utf8_str.c_str());
    AtlasGlyph atlas_glyph = atlas.insertGlyph(glyph);
    glyph_cache.insert({utf8_str, atlas_glyph});
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
