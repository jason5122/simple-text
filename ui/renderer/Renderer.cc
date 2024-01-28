#include "Renderer.h"
#include "third_party/tree_sitter/include/tree_sitter/api.h"
#include "util/FileUtil.h"
#include "util/LogUtil.h"
#include "util/OpenGLErrorUtil.h"

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
};

extern "C" TSLanguage* tree_sitter_json();

void Renderer::treeSitterExperiment() {
    TSParser* parser = ts_parser_new();

    // Set the parser's language (JSON in this case).
    ts_parser_set_language(parser, tree_sitter_json());

    // Build a syntax tree based on source code stored in a string.
    const char* source_code = ReadFile(ResourcePath("sample_files/larger_example.json"));
    TSTree* tree = ts_parser_parse_string(parser, NULL, source_code, strlen(source_code));

    // Get the root node of the syntax tree.
    TSNode root_node = ts_tree_root_node(tree);

    // Print the syntax tree as an S-expression.
    // char* string = ts_node_string(root_node);
    // LogDefault("Renderer", "Syntax tree: \n%s", string);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    const char* query_code = ReadFile(ResourcePath("highlights.scm"));
    TSQuery* query = ts_query_new(tree_sitter_json(), query_code, strlen(query_code),
                                  &error_offset, &error_type);

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

    TSQueryCursor* query_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(query_cursor, query, root_node);

    const void* prev_id = 0;
    uint32_t prev_start = -1;
    uint32_t prev_end = -1;

    // TODO: Profile this code and optimize it to be as fast as Tree-sitter's CLI.
    TSQueryMatch match;
    uint32_t capture_index;
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

    // Free all of the heap-allocated memory.
    // free(string);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

Renderer::Renderer(float width, float height, std::string main_font_name,
                   std::string emoji_font_name, int font_size) {
    rasterizer = new Rasterizer(main_font_name, emoji_font_name, font_size);
    atlas_renderer = new AtlasRenderer(width, height);

    Metrics metrics = rasterizer->metrics();
    float cell_width = std::floor(metrics.average_advance + 1);
    float cell_height = std::floor(metrics.line_height + 2);

    cursor_renderer = new CursorRenderer(width, height, cell_width, cell_height);

    this->linkShaders();
    this->resize(width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    this->treeSitterExperiment();
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault("Renderer", "Tree-sitter: %ld µs (%f fps)", microseconds, fps);

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

    // Preload common glyphs.
    // for (int i = 32; i < 127; i++) {
    //     char ch = static_cast<char>(i);
    //     this->loadGlyph(ch);
    // }

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

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::renderText(Buffer& buffer, float scroll_x, float scroll_y, float cursor_x,
                          float cursor_y, float drag_x, float drag_y) {
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "scroll_offset"), scroll_x, scroll_y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;
    uint32_t byte_offset = 0;
    int range_idx = 0;

    bool infinite_scroll = true;

    Metrics metrics = rasterizer->metrics();
    float cell_width = std::floor(metrics.average_advance + 1);
    float cell_height = std::floor(metrics.line_height + 2);

    int row_offset = scroll_y / -cell_height;
    if (row_offset < 0) row_offset = 0;

    if (infinite_scroll) {
        for (uint16_t row = 0; row < std::min(row_offset, static_cast<int>(buffer.lineCount()));
             row++) {
            for (uint16_t col = 0; col < buffer.data[row].size(); col++) {
                byte_offset++;
            }
            byte_offset++;
        }
    } else {
        row_offset = 0;
    }

    uint16_t size = infinite_scroll
                        ? std::min(row_offset + 60, static_cast<int>(buffer.lineCount()))
                        : buffer.lineCount();

    uint16_t cursor_col = round(cursor_x / cell_width);
    uint16_t cursor_row = (height - cursor_y) / cell_height;

    uint16_t drag_col = round(drag_x / cell_width);
    uint16_t drag_row = (height - drag_y) / cell_height;

    // LogDefault("Renderer", "pixels: %f %f", cursor_x, height - cursor_y);
    // LogDefault("Renderer", "cursor: (%d, %d)", cursor_col, cursor_row);
    // LogDefault("Renderer", "drag: (%d, %d)", drag_col, drag_row);

    uint16_t start_row, start_col;
    uint16_t end_row, end_col;
    if (cursor_row < drag_row) {
        start_row = cursor_row;
        start_col = cursor_col;
        end_row = drag_row;
        end_col = drag_col;
    } else {
        start_row = drag_row;
        start_col = drag_col;
        end_row = cursor_row;
        end_col = cursor_col;
    }

    for (uint16_t row = row_offset; row < size; row++) {
        float total_advance = 0;
        for (uint16_t col = 0; col < buffer.data[row].size(); col++) {
            char ch = buffer.data[row][col];

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

            uint8_t bg_a = 0;
            if (start_row < row && row < end_row || row == start_row && col >= start_col ||
                row == end_row && col < end_col) {
                bg_a = 255;
            }

            if (!glyph_cache.count(ch)) {
                this->loadGlyph(ch);
            }

            AtlasGlyph glyph = glyph_cache[ch];
            instances.push_back(InstanceData{
                // Grid coordinates.
                col,
                row,
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
                static_cast<float>(std::round(total_advance)),
                // Background color.
                YELLOW.r,
                YELLOW.g,
                YELLOW.b,
                bg_a,
            });

            total_advance += glyph.advance;
            // FIXME: Hack to render almost like Sublime Text (pretty much pixel perfect!).
            if (rasterizer->isFontMonospace()) {
                total_advance = std::round(total_advance + 1);
            }

            byte_offset++;
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

    cursor_renderer->draw(scroll_x, scroll_y, drag_col, drag_row);
    glEnable(GL_BLEND);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();
}

void Renderer::loadGlyph(char ch) {
    bool emoji = ch == '@' ? true : false;  // DEBUG: Emoji testing hack.
    RasterizedGlyph glyph = rasterizer->rasterizeChar(ch, emoji);
    AtlasGlyph atlas_glyph = atlas.insertGlyph(glyph);
    glyph_cache.insert({ch, atlas_glyph});
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
