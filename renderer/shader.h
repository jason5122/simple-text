#pragma once

#include "util/non_copyable.h"
#include <glad/glad.h>
#include <string>

namespace renderer {

class Shader : util::NonMovable {
public:
    GLuint id;

    Shader();
    ~Shader();
    bool link(const std::string& vert_source, const std::string& frag_source);
};

}
