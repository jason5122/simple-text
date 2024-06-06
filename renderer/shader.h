#pragma once

#include "util/file_util.h"

#include "build/buildflag.h"
#if IS_MAC
#include <OpenGL/gl3.h>
#else
#include <epoxy/gl.h>
#endif

class Shader {
public:
    GLuint id;

    Shader() = default;
    bool link(fs::path vert_path, fs::path frag_path);
    ~Shader();
};
