#pragma once

#include "gl/gl.h"
#include <string>

namespace gui {

class Shader {
public:
    Shader(const std::string& vert_source, const std::string& frag_source);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    gl::GLuint id();

private:
    gl::GLuint id_ = 0;
};

static_assert(std::movable<Shader>);
static_assert(!std::copyable<Shader>);

}  // namespace gui
