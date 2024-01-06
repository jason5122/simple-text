#import "OpenGLLayer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import <OpenGL/gl3.h>

@interface OpenGLLayer () {
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;
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
        // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_designstrategies/opengl_designstrategies.html#//apple_ref/doc/uid/TP40001987-CH2-SW4
        GLint params = 1;
        CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &params);

        CGLSetCurrentContext(context);

        logDefault(@"OpenGL", @"%s", glGetString(GL_VERSION));

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

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float vertices[] = {
            0.0f, 0.0f,  //
            0.5f, 0.5f,  //
            0.0f, 0.5f,  //
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind so future calls won't modify this VAO/VBO.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    return context;
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return YES;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    CGLSetCurrentContext(glContext);

    logDefault(@"OpenGL", @"%fx%f", self.frame.size.width, self.frame.size.height);

    glViewport(0, 0, 1728, 1051);

    GLuint uViewportSize = glGetUniformLocation(shaderProgram, "viewportSize");
    GLfloat w = self.frame.size.width;
    GLfloat h = self.frame.size.height;
    glUniform2fv(uViewportSize, 2, new float[2]{w, h});

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

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
    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

@end
