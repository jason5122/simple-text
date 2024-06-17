#pragma once

#include "opengl/functions_gl.h"
#include "opengl/functions_gl_typedefs.h"
#include "util/non_copyable.h"
#include <string>

namespace renderer {

class Shader : util::NonCopyable {
public:
    Shader(std::shared_ptr<opengl::FunctionsGL> shared_gl);
    ~Shader();
    Shader(Shader&& other);
    Shader& operator=(Shader&& other);

    GLuint id();
    bool link(const std::string& vert_source, const std::string& frag_source);

private:
    std::shared_ptr<opengl::FunctionsGL> gl;

    GLuint id_ = 0;
};

}
