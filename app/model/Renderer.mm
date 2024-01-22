#import "Renderer.h"
#import "util/CTFontUtil.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import "util/OpenGLErrorUtil.h"

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
    uint8_t colored;
};

Renderer::Renderer(float width, float height, CTFontRef mainFont)
    : width(width), height(height), mainFont(mainFont) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    this->linkShaders();

    Metrics metrics = CTFontGetMetrics(mainFont);
    float cell_width = CGFloat_floor(metrics.average_advance + 1);
    float cell_height = CGFloat_floor(metrics.line_height + 2);

    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "resolution"), width, height);
    glUniform2f(glGetUniformLocation(shader_program, "cell_dim"), cell_width, cell_height);

    // Font experiments.
    emojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 36 * 2, nullptr);
    LogDefault(@"Renderer", @"colored? %d", CTFontIsColored(emojiFont));

    NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Menlo"};
    CTFontDescriptorRef descriptor =
        CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
    CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
    CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
    CFArrayRef fontDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, NULL);

    for (int i = 0; i < CFArrayGetCount(fontDescriptors); i++) {
        CTFontDescriptorRef descriptor =
            (CTFontDescriptorRef)CFArrayGetValueAtIndex(fontDescriptors, i);
        CFStringRef familyName =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
        CFStringRef style =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);
        LogDefault(@"Renderer", @"%@ %@", familyName, style);

        CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, 32, nullptr);
    }

    CGGlyph glyph_index = CTFontGetEmojiGlyphIndex(emojiFont);
    LogDefault(@"Renderer", @"index: %d", glyph_index);
    // End of font experiments.

    this->createAtlas();

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
    size += sizeof(uint8_t);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    atlas_renderer = new AtlasRenderer(width, height);
}

void Renderer::renderText(std::string text, float x, float y) {
    glUseProgram(shader_program);
    glUniform2f(glGetUniformLocation(shader_program, "scroll_offset"), x, y);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    std::vector<InstanceData> instances;
    for (uint16_t col = 0; col < text.size(); col++) {
        char ch = text[col];

        if (!glyph_cache.count(ch)) {
            this->loadGlyph(ch);
        }

        AtlasGlyph glyph = glyph_cache[ch];
        instances.push_back(InstanceData{
            // grid_coords
            col,
            0,
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
            glyph.colored,
        });
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(InstanceData) * instances.size(), &instances[0]);

    glBindTexture(GL_TEXTURE_2D, atlas);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();

    atlas_renderer->draw(x + ATLAS_SIZE, y, atlas, ATLAS_SIZE);

    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    atlas_renderer->draw(x + ATLAS_SIZE, y, atlas, ATLAS_SIZE);
    glEnable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::createAtlas() {
    // rasterizer = new Rasterizer(mainFont);
    // rasterizer = new Rasterizer(emojiFont);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    offset_x = 0;
    offset_y = 0;
    tallest = 0;

    // Preload common glyphs.
    // for (int i = 32; i < 127; i++) {
    //     char ch = static_cast<char>(i);
    //     this->loadGlyph(ch);
    // }
}

void Renderer::loadGlyph(char ch) {
    bool emoji = ch == '-' ? true : false;

    CGGlyph glyph_index;
    RasterizedGlyph glyph;
    if (emoji) {
        glyph_index = CTFontGetEmojiGlyphIndex(emojiFont);
        glyph = rasterizer.rasterizeGlyph(glyph_index, emojiFont);
    } else {
        glyph_index = CTFontGetGlyphIndex(mainFont, ch);
        glyph = rasterizer.rasterizeGlyph(glyph_index, mainFont);
    }

    tallest = std::max(glyph.height, tallest);
    if (offset_x + glyph.width > ATLAS_SIZE) {
        offset_x = 0;
        offset_y += tallest;
        tallest = 0;
    }

    glBindTexture(GL_TEXTURE_2D, atlas);

    GLenum format = glyph.colored ? GL_RGBA : GL_RGB;
    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, glyph.width, glyph.height, format,
                    GL_UNSIGNED_BYTE, &glyph.buffer[0]);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.

    float uv_left = static_cast<float>(offset_x) / ATLAS_SIZE;
    float uv_bot = static_cast<float>(offset_y) / ATLAS_SIZE;
    float uv_width = static_cast<float>(glyph.width) / ATLAS_SIZE;
    float uv_height = static_cast<float>(glyph.height) / ATLAS_SIZE;

    AtlasGlyph atlas_glyph = {
        glyph.colored, glyph.left, glyph.top, glyph.width, glyph.height,
        uv_left,       uv_bot,     uv_width,  uv_height,
    };
    glyph_cache.insert({ch, atlas_glyph});

    offset_x += glyph.width;
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
