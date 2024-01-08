#import "OpenGLLayer.h"
#import "model/Atlas.h"
#import "model/Font.h"
#import "model/Rasterizer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import <OpenGL/gl3.h>

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

@interface OpenGLLayer () {
    GLuint shaderProgram;
    GLuint vao;
    GLuint ebo;
    GLuint vbo_instance;
    GLfloat screenWidth, screenHeight;
    GLint u_projection;
    GLint u_cell_dim;
    SizeInfo size_info;
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

        // glEnable(GL_BLEND);
        // glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        // glDepthMask(GL_FALSE);

        // glGenVertexArrays(1, &vao);
        // glGenBuffers(1, &ebo);
        // glGenBuffers(1, &vbo_instance);
        // glBindVertexArray(vao);

        // uint32_t indices[] = {0, 1, 3, 1, 2, 3};
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), indices, GL_STATIC_DRAW);

        // glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
        // glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(InstanceData), nullptr, GL_STREAM_DRAW);

        // GLuint index = 0;
        // GLuint size = 0;

        // glVertexAttribPointer(index, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(InstanceData),
        // &size); glEnableVertexAttribArray(index); glVertexAttribDivisor(index, 1); index++; size
        // += 2 * sizeof(uint16_t);

        // glVertexAttribPointer(index, 4, GL_SHORT, GL_FALSE, sizeof(InstanceData), &size);
        // glEnableVertexAttribArray(index);
        // glVertexAttribDivisor(index, 1);
        // index++;
        // size += 4 * sizeof(int16_t);

        // glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), &size);
        // glEnableVertexAttribArray(index);
        // glVertexAttribDivisor(index, 1);
        // index++;
        // size += 4 * sizeof(float);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo_instance);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);

        float gray = 228 / 255.f;
        float darkGray = 207 / 255.f;
        float vertices[] = {
            // Tab bar
            0, 1051 - 30, gray, gray, gray,     //
            1728, 1051, gray, gray, gray,       //
            0, 1051, gray, gray, gray,          //
            1728, 1051 - 30, gray, gray, gray,  //
            1728, 1051, gray, gray, gray,       //
            0, 1051 - 30, gray, gray, gray,     //

            // Side bar
            200, 0, gray, gray, gray,     //
            0, 1051, gray, gray, gray,    //
            0, 0, gray, gray, gray,       //
            200, 1051, gray, gray, gray,  //
            0, 1051, gray, gray, gray,    //
            200, 0, gray, gray, gray,     //

            // Status bar
            0, 50, darkGray, darkGray, darkGray,     //
            1728, 0, darkGray, darkGray, darkGray,   //
            0, 0, darkGray, darkGray, darkGray,      //
            1728, 50, darkGray, darkGray, darkGray,  //
            1728, 0, darkGray, darkGray, darkGray,   //
            0, 50, darkGray, darkGray, darkGray,     //
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Unbind so future calls won't modify this VAO/VBO.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        [self draw];  // Initial draw call.
    }
    return context;
}

- (void)draw {
    // glUseProgram(shaderProgram);
    // glUniform2f(u_cell_dim, size_info.cell_width, size_info.cell_height);

    // glBindVertexArray(vao);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    // glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    // glActiveTexture(GL_TEXTURE0);

    // std::vector<InstanceData> instances;

    // Font menlo = Font(CFSTR("Menlo"), 32);
    // Atlas atlas = Atlas(1024);
    // Rasterizer rasterizer = Rasterizer();
    // CGGlyph glyph_index = menlo.get_glyph(@"E");
    // RasterizedGlyph rasterized_glyph = rasterizer.rasterize_glyph(glyph_index);
    // Glyph glyph = atlas.insert_inner(rasterized_glyph);
    // instances.push_back(InstanceData{0, 10, glyph.left, glyph.top, glyph.width, glyph.height,
    //                                  glyph.uv_left, glyph.uv_bot, glyph.uv_width,
    //                                  glyph.uv_height});

    // glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(InstanceData),
    // instances.data()); glBindTexture(GL_TEXTURE_2D, glyph.tex_id);
    // glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances.size());

    glViewport(0, 0, screenWidth, screenHeight);

    GLuint uResolution = glGetUniformLocation(shaderProgram, "resolution");
    glUniform2fv(uResolution, 1, new GLfloat[]{screenWidth, screenHeight});

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 18);
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
