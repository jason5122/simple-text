#pragma once

#include "renderer/opengl_functions.h"
#include "util/non_copyable.h"
#include <string>

namespace renderer {

class Shader {
public:
    GLuint id;

    Shader();
    ~Shader();
    bool link(const std::string& vert_source, const std::string& frag_source);
};

}
