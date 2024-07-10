#pragma once

#include "opengl/gl.h"
#include "util/non_copyable.h"
#include <string>

namespace renderer {

class Shader : util::NonCopyable {
public:
    Shader(const std::string& vert_source, const std::string& frag_source);
    ~Shader();
    Shader(Shader&& other);
    Shader& operator=(Shader&& other);

    GLuint id();

private:
    GLuint id_ = 0;
};

}
