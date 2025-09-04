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

static_assert(std::is_nothrow_destructible_v<Shader>);
static_assert(!std::is_copy_constructible_v<Shader>);
static_assert(!std::is_copy_assignable_v<Shader>);
static_assert(std::is_nothrow_move_constructible_v<Shader>);
static_assert(std::is_nothrow_move_assignable_v<Shader>);

}  // namespace gui
