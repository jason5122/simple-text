#pragma once

#include "gl/gl.h"
#include "util/non_copyable.h"
#include <string>

namespace gui {

class Shader : util::NonCopyable {
public:
    Shader(const std::string& vert_source, const std::string& frag_source);
    ~Shader();
    Shader(Shader&& other);
    Shader& operator=(Shader&& other);

    gl::GLuint id();

private:
    gl::GLuint id_ = 0;
};

}  // namespace gui
