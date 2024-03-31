#pragma once

#include "util/file_util.h"
#include <glad/glad.h>

class Shader {
public:
    GLuint id;

    Shader() = default;
    bool link(fs::path vert_path, fs::path frag_path);
    ~Shader();
};
