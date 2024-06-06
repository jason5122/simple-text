#pragma once

#include "util/file_util.h"
#include <OpenGL/gl3.h>

class Shader {
public:
    GLuint id;

    Shader() = default;
    bool link(std::string& vert_source, std::string& frag_source);
    ~Shader();
};
