#pragma once

#include <glad/glad.h>
#include <string>

namespace renderer {
class Shader {
public:
    GLuint id;

    Shader();
    ~Shader();
    bool link(std::string& vert_source, std::string& frag_source);
};
}
