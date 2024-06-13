#pragma once

#include "opengl/functions_gl.h"
#include "opengl/functionsgl_typedefs.h"
#include "util/non_copyable.h"
#include <string>

namespace renderer {

class Shader : util::NonCopyable {
public:
    Shader(opengl::FunctionsGL* gl);
    ~Shader();
    Shader(Shader&&);
    Shader& operator=(Shader&&);

    GLuint id();
    bool link(const std::string& vert_source, const std::string& frag_source);

private:
    opengl::FunctionsGL* gl;

    GLuint id_;
};

}
