#import "Renderer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import "util/OpenGLErrorUtil.h"
#include <tree_sitter/api.h>

struct InstanceData {
    // grid_coords
    uint16_t col;
    uint16_t row;
    // glyph
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    // uv
    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
    // flags
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t colored;
};

extern "C" TSLanguage* tree_sitter_json();

void Renderer::treeSitterExperiment() {
    TSParser* parser = ts_parser_new();

    // Set the parser's language (JSON in this case).
    ts_parser_set_language(parser, tree_sitter_json());

    // Build a syntax tree based on source code stored in a string.
    const char* source_code = ReadFile(ResourcePath("larger_example.json"));
    TSTree* tree = ts_parser_parse_string(parser, NULL, source_code, strlen(source_code));

    // Get the root node of the syntax tree.
    TSNode root_node = ts_tree_root_node(tree);

    // Print the syntax tree as an S-expression.
    // char* string = ts_node_string(root_node);
    // LogDefault(@"Renderer", @"Syntax tree: \n%s", string);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;
    const char* query_code = ReadFile(ResourcePath("highlights.scm"));
    TSQuery* query = ts_query_new(tree_sitter_json(), query_code, strlen(query_code),
                                  &error_offset, &error_type);

    if (error_type != TSQueryErrorNone) {
        LogError(@"Renderer", @"Error creating new TSQuery. error_offset: %d, error type: %d",
                 error_offset, error_type);
    }

    std::vector<const char*> capture_names;
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
            // LogDefault(@"Renderer", @"%d, [%d, %d], %s", node.id, start_byte, end_byte,
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
                   std::string emoji_font_name, int font_size)
    : width(width), height(height) {
    rasterizer = new Rasterizer(main_font_name, emoji_font_name, font_size);
    atlas_renderer = new AtlasRenderer(width, height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    uint64_t start = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    this->treeSitterExperiment();
    uint64_t end = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    uint64_t microseconds = (end - start) / 1e3;
    float fps = 1000000.0 / microseconds;
    LogDefault(@"Renderer", @"Tree-sitter: %ld µs (%f fps)", microseconds, fps);

    this->linkShaders();

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
    //         LogDefault(@"Renderer", @"%@ %@", familyName, style);
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

    Metrics metrics = rasterizer->metrics;
    float cell_width = CGFloat_floor(metrics.average_advance + 1);
    float cell_height = CGFloat_floor(metrics.line_height + 2);

    LogDefault(@"Renderer", @"cell_height: %f", cell_height);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);
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

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::renderText(std::vector<std::string> text, float x, float y, uint16_t row_offset) {
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "scroll_offset"), x, y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;
    uint32_t byte_offset = 0;
    int range_idx = 0;

    bool infinite_scroll = true;

    if (infinite_scroll) {
        for (uint16_t row = 0; row < std::min(row_offset, static_cast<uint16_t>(text.size()));
             row++) {
            for (uint16_t col = 0; col < text[row].size(); col++) {
                byte_offset++;
            }
            byte_offset++;
        }
    } else {
        row_offset = 0;
    }

    uint16_t size =
        infinite_scroll ? std::min(row_offset + 35, static_cast<int>(text.size())) : text.size();

    for (uint16_t row = row_offset; row < size; row++) {
        for (uint16_t col = 0; col < text[row].size(); col++) {
            char ch = text[row][col];

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

            if (!glyph_cache.count(ch)) {
                this->loadGlyph(ch);
            }

            AtlasGlyph glyph = glyph_cache[ch];
            instances.push_back(InstanceData{
                // grid_coords
                col,
                row,
                // glyph
                glyph.left,
                glyph.top,
                glyph.width,
                glyph.height,
                // uv
                glyph.uv_left,
                glyph.uv_bot,
                glyph.uv_width,
                glyph.uv_height,
                // flags
                text_color.r,
                text_color.g,
                text_color.b,
                glyph.colored,
            });

            byte_offset++;
        }
        byte_offset++;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas.tex_id);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();

    atlas_renderer->draw(x + 1700.0f, y, atlas.tex_id);

    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    atlas_renderer->draw(x + 1700.0f, y, atlas.tex_id);
    glEnable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::loadGlyph(char ch) {
    bool emoji = ch == '$' ? true : false;  // DEBUG: Emoji testing hack.
    RasterizedGlyph glyph = rasterizer->rasterizeChar(ch, emoji);
    AtlasGlyph atlas_glyph = atlas.insertGlyph(glyph);
    glyph_cache.insert({ch, atlas_glyph});
}

void Renderer::clearAndResize() {
    glViewport(0, 0, width, height);
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::linkShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vert_source = ReadFile(ResourcePath("text_vert.glsl"));
    const GLchar* frag_source = ReadFile(ResourcePath("text_frag.glsl"));
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
