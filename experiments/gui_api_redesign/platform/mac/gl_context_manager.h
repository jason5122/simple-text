#pragma once

#include <OpenGL/OpenGL.h>
#include <memory>

class GLContextManager {
public:
    ~GLContextManager();
    GLContextManager(const GLContextManager&) = delete;
    GLContextManager& operator=(const GLContextManager&) = delete;
    GLContextManager(GLContextManager&&) = delete;
    GLContextManager& operator=(GLContextManager&&) = delete;
    static std::unique_ptr<GLContextManager> create();

    CGLPixelFormatObj pixel_format() const { return pixel_format_; }
    CGLContextObj shared_context() const { return shared_context_; }
    CGLContextObj create_layer_context() const;

private:
    GLContextManager() = default;
    CGLPixelFormatObj pixel_format_ = nullptr;
    CGLContextObj shared_context_ = nullptr;
};
