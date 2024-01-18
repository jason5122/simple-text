#import "Renderer.h"
#import "model/Font.h"
#import "model/Rasterizer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"

Renderer::Renderer(float width, float height) : width(width), height(height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
    glDepthMask(GL_FALSE);

    link_shaders();

    glUseProgram(shaderProgram);
    glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), width, height);

    bool success = load_glyphs();
    if (!success) {
        logDefault(@"Renderer", @"error loading font glyphs");
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool Renderer::load_glyphs() {
    Font menlo = Font(CFSTR("Menlo"), 48);
    Rasterizer rasterizer = Rasterizer();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char ch = 'E'; ch <= 'E'; ch++) {
        CGGlyph glyph_index = menlo.get_glyph([NSString stringWithFormat:@"%c", ch]);
        RasterizedGlyph rasterized_glyph = rasterizer.rasterize_glyph(glyph_index);

        GLsizei width = rasterized_glyph.width;
        GLsizei height = rasterized_glyph.height;
        GLsizei top = rasterized_glyph.top;
        GLsizei left = rasterized_glyph.left;
        uint8_t* buffer = &rasterized_glyph.buffer[0];

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {texture, glm::ivec2(width, height), glm::ivec2(left, top)};
        characters.insert({ch, character});
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Renderer::render_text(std::string text, float x, float y) {
    glViewport(0, 0, width, height);
    glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    Font menlo = Font(CFSTR("Menlo"), 48);
    Metrics metrics = menlo.metrics();
    float advance = metrics.average_advance;

    for (const char c : text) {
        Character ch = characters[c];

        float xpos = x + ch.bearing.x;
        float ypos = y - (ch.size.y - ch.bearing.y);

        float w = ch.size.x;
        float h = ch.size.y;

        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos, ypos, 0.0f, 1.0f},          //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos + w, ypos + h, 1.0f, 0.0f},  //
        };
        glBindTexture(GL_TEXTURE_2D, ch.tex_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += advance;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
