#include <glad/glad.h>

class TriangleRenderer {
public:
    TriangleRenderer() = default;
    void setup();
    void draw();

private:
    GLuint shader_program;
    GLuint vao, vbo;

    void linkShaders();
};
