#pragma once

#include <OpenGL/gl3.h>
#include <string>

class Shader {
public:
    GLuint id;

    Shader() = default;
    bool link(const std::string& vert_source, const std::string& frag_source);
    ~Shader();
};
