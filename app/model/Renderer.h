#import <OpenGL/gl3.h>
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>
#import <map>

struct Character {
    GLuint tex_id;       // ID handle of the glyph texture
    glm::ivec2 size;     // size of glyph
    glm::ivec2 bearing;  // Offset from baseline to left/top of glyph
    long advance;        // Horizontal offset to advance to next glyph
};

class Renderer {
public:
    Renderer(float width, float height);
    void init();
    void render_text(std::string text, float x, float y, float scale, glm::vec3 color);
    ~Renderer();

private:
    GLfloat width, height;
    GLuint shaderProgram;
    GLuint VAO, VBO;
    std::map<GLchar, Character> characters;

    void setup_shaders();
    bool load_glyphs();
};
