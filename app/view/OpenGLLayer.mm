#import "OpenGLLayer.h"
#import "model/Atlas.h"
#import "model/Font.h"
#import "model/Rasterizer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>

#include <ft2build.h>
#include FT_FREETYPE_H

struct SizeInfo {
    float width;
    float height;
    float cell_width;
    float cell_height;
    float padding_x;
    float padding_y;
};

struct InstanceData {
    uint16_t col;
    uint16_t row;

    int16_t left;
    int16_t top;
    int16_t width;
    int16_t height;

    float uv_left;
    float uv_bot;
    float uv_width;
    float uv_height;
};

struct Character {
    unsigned int tex_id;   // ID handle of the glyph texture
    glm::ivec2 size;       // size of glyph
    glm::ivec2 bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;  // Horizontal offset to advance to next glyph
};

@interface OpenGLLayer () {
    GLuint shaderProgram;
    GLuint vao;
    GLuint ebo;
    GLuint vbo_instance;
    GLfloat screenWidth, screenHeight;
    GLint u_projection;
    GLint u_cell_dim;
    SizeInfo size_info;

    std::map<GLchar, Character> characters;
    unsigned int VAO, VBO;
}
@end

@implementation OpenGLLayer

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFADisplayMask,
        static_cast<CGLPixelFormatAttribute>(mask),
        kCGLPFAColorSize,
        static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAlphaSize,
        static_cast<CGLPixelFormatAttribute>(8),
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0),
    };

    CGLPixelFormatObj pixelFormat = nullptr;
    GLint numFormats = 0;
    CGLChoosePixelFormat(attribs, &pixelFormat, &numFormats);
    return pixelFormat;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj context = nullptr;
    CGLCreateContext(pixelFormat, nullptr, &context);
    if (context || (context = [super copyCGLContextForPixelFormat:pixelFormat])) {
        screenWidth = NSScreen.mainScreen.frame.size.width;
        screenHeight = NSScreen.mainScreen.frame.size.height;

        // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_designstrategies/opengl_designstrategies.html#//apple_ref/doc/uid/TP40001987-CH2-SW4
        // GLint params = 1;
        // CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &params);

        CGLSetCurrentContext(context);

        // Font menlo = Font(CFSTR("Menlo"), 32);
        // Metrics metrics = menlo.metrics();
        // logDefault(@"OpenGL", @"average_advance = %f, line_height = %f",
        // metrics.average_advance,
        //            metrics.line_height);

        // float cell_width = floor(metrics.average_advance + 1);
        // float cell_height = floor(metrics.line_height + 2);
        // logDefault(@"OpenGL", @"cell_width = %f, cell_height = %f", cell_width, cell_height);

        // Rasterizer rasterizer = Rasterizer();
        // CGGlyph glyph_index = menlo.get_glyph(@"E");
        // RasterizedGlyph rasterized_glyph = rasterizer.rasterize_glyph(glyph_index);

        // Atlas atlas = Atlas(1024);
        // Glyph glyph = atlas.insert_inner(rasterized_glyph);

        // logDefault(@"OpenGL", @"%s", glGetString(GL_VERSION));
        // logDefault(@"OpenGL", @"%fx%f", screenWidth, screenHeight);

        // logDefault(@"OpenGL", @"viewport_size: %fx%f", self.frame.size.width,
        //            self.frame.size.height);
        // float viewport_width = self.frame.size.width;
        // float viewport_height = self.frame.size.height;
        // size_info = {viewport_width * 2, viewport_height * 2, cell_width,
        // cell_height, 10.0, 10.0};

        // glViewport(size_info.padding_x, size_info.padding_y,
        //            size_info.width - 2 * size_info.padding_x,
        //            size_info.height - 2 * size_info.padding_y);
        // glUseProgram(shaderProgram);

        // u_projection = glGetUniformLocation(shaderProgram, "projection");
        // u_cell_dim = glGetUniformLocation(shaderProgram, "cellDim");

        // float scale_x = 2.0 / (size_info.width - 2.0 * size_info.padding_x);
        // float scale_y = -2.0 / (size_info.height - 2.0 * size_info.padding_y);
        // float offset_x = -1.0;
        // float offset_y = 1.0;
        // logDefault(@"OpenGL", @"%f %f", scale_x, scale_y);
        // glUniform4f(u_projection, offset_x, offset_y, scale_x, scale_y);
        // glUseProgram(0);

        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar* vertSource = readFile(resourcePath("triangle_vert.glsl"));
        const GLchar* fragSource = readFile(resourcePath("triangle_frag.glsl"));
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

        glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));

        bool success = [self loadGlyphs];
        if (!success) {
            logDefault(@"OpenGL", @"error loading font glyphs");
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        [self draw];  // Initial draw call.
    }
    return context;
}

- (bool)loadGlyphs {
    FT_Error error;

    FT_Library ft;
    error = FT_Init_FreeType(&ft);
    if (error != FT_Err_Ok) {
        logDefault(@"OpenGL", @"%s", FT_Error_String(error));
        return false;
    }

    FT_Face face;
    error = FT_New_Face(ft, resourcePath("Antonio-Regular.ttf"), 0, &face);
    if (error != FT_Err_Ok) {
        logDefault(@"OpenGL", @"%s", FT_Error_String(error));
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        error = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if (error != FT_Err_Ok) {
            logDefault(@"OpenGL", @"%s", FT_Error_String(error));
            return false;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {texture,
                               glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                               glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                               static_cast<unsigned int>(face->glyph->advance.x)};
        characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

- (void)draw {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    [self renderText:"This is sample text"
                   x:25.0f
                   y:25.0f
               scale:1.0f
               color:glm::vec3(0.5, 0.8f, 0.2f)];
    [self renderText:"This is sample text"
                   x:540.0f
                   y:570.0f
               scale:0.5f
               color:glm::vec3(0.3, 0.7f, 0.9f)];
}

- (void)renderText:(std::string)text
                 x:(float)x
                 y:(float)y
             scale:(float)scale
             color:(glm::vec3)color {
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (const char c : text) {
        Character ch = characters[c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos, ypos, 0.0f, 1.0f},          //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos, ypos + h, 0.0f, 0.0f},      //
            {xpos + w, ypos, 1.0f, 1.0f},      //
            {xpos + w, ypos + h, 1.0f, 0.0f},  //
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.tex_id);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                        vertices);  // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.advance >> 6) * scale;  // bitshift by 6 to get value in pixels (2^6 = 64 (divide
                                         // amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    CGLSetCurrentContext(glContext);

    [self draw];

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

- (void)dealloc {
    // glDeleteProgram(shaderProgram);
    // glDeleteVertexArrays(1, &vao);
    // glDeleteBuffers(1, &vbo_instance);
}

@end
