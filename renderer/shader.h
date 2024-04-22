#pragma once

#include "base/filesystem/file_reader.h"
#include <glad/glad.h>

class Shader {
public:
    GLuint id;

    Shader() = default;
    bool link(std::string& vert_source, std::string& frag_source);
    ~Shader();
};
