#pragma once

#include "util/not_copyable_or_movable.h"
#include <glad/glad.h>
#include <string>

namespace renderer {
class Shader {
public:
    GLuint id;

    NOT_COPYABLE(Shader)
    NOT_MOVABLE(Shader)
    Shader();
    ~Shader();
    bool link(const std::string& vert_source, const std::string& frag_source);
};
}
