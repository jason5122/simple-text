#import "Renderer.h"
#import "model/Rasterizer.h"
#import "util/CTFontUtil.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import "util/OpenGLErrorUtil.h"

Renderer::Renderer(float width, float height, CTFontRef mainFont)
    : width(width), height(height), mainFont(mainFont) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // DEBUG: Draw shapes as wireframes.

    link_shaders();

    glUseProgram(shaderProgram);
    glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), width, height);

    // Font experiments.
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, nullptr);
    logDefault(@"Renderer", @"colored? %d", CTFontIsColored(appleEmojiFont));

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
        logDefault(@"Renderer", @"%@ %@", familyName, style);

        CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, 32, nullptr);
    }
    // End of font experiments.

    load_glyphs();

    glm::vec2 translations[3];
    translations[0] = glm::vec2(0.0f, 0.0f);
    translations[1] = glm::vec2(50.0f, 0.0f);
    translations[2] = glm::vec2(100.0f, 0.0f);

    glGenBuffers(1, &VBO_instance);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 3, &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3,  // second triangle
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_instance);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(1, 1);

    // Unbind.
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::load_glyphs() {
    Rasterizer rasterizer = Rasterizer(mainFont);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 32; i < 127; i++) {
        char ch = static_cast<char>(i);
        CGGlyph glyph_index = CTFontGetGlyphIndex(mainFont, ch);
        RasterizedGlyph glyph = rasterizer.rasterize_glyph(glyph_index);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, glyph.width, glyph.height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, &glyph.buffer[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {texture, glm::ivec2(glyph.width, glyph.height),
                               glm::ivec2(glyph.left, glyph.top)};
        characters.insert({ch, character});
    }
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind.
}

void Renderer::render_text(std::string text, float x, float y) {
    // glViewport(0, 0, width, height);
    // glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    Metrics metrics = CTFontGetMetrics(mainFont);

    Character ch = characters[text[0]];
    float xpos = x + ch.bearing.x;
    float ypos = y - (ch.size.y - ch.bearing.y);
    float w = ch.size.x;
    float h = ch.size.y;
    float vertices[4][4] = {
        {xpos + w, ypos + h, 1.0f, 0.0f},  // bottom right
        {xpos + w, ypos, 1.0f, 1.0f},      // top right
        {xpos, ypos, 0.0f, 1.0f},          // top left
        {xpos, ypos + h, 0.0f, 0.0f},      // bottom left
    };
    glBindTexture(GL_TEXTURE_2D, ch.tex_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.

    // for (const char c : text) {
    //     Character ch = characters[c];

    //     float xpos = x + ch.bearing.x;
    //     float ypos = y - (ch.size.y - ch.bearing.y);
    //     float w = ch.size.x;
    //     float h = ch.size.y;

    //     float vertices[4][4] = {
    //         {xpos + w, ypos + h, 1.0f, 0.0f},  // bottom right
    //         {xpos + w, ypos, 1.0f, 1.0f},      // top right
    //         {xpos, ypos, 0.0f, 1.0f},          // top left
    //         {xpos, ypos + h, 0.0f, 0.0f},      // bottom left
    //     };

    //     glBindTexture(GL_TEXTURE_2D, ch.tex_id);
    //     glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //     glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    //     // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //     glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, 3);

    //     glBindBuffer(GL_ARRAY_BUFFER, 0);  // Unbind.
    //     x += metrics.average_advance;      // FIXME: Assumes font is monospaced.
    // }
    // Unbind.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // DEBUG: If this shows an error, keep moving this up until the problematic line is found.
    // https://learnopengl.com/In-Practice/Debugging
    glPrintError();
}

void Renderer::link_shaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* vertSource = readFile(resourcePath("text_vert.glsl"));
    const GLchar* fragSource = readFile(resourcePath("text_frag.glsl"));
    glShaderSource(vertexShader, 1, &vertSource, nullptr);
    glShaderSource(fragmentShader, 1, &fragSource, nullptr);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}
