#import "Renderer.h"
#import "util/FileUtil.h"
#import "util/LogUtil.h"
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

Renderer::Renderer(float width, float height) {
    this->width = width;
    this->height = height;
}

bool Renderer::init() {
    FT_Library ft;
    FT_Init_FreeType(&ft);

    FT_Face face;
    FT_New_Face(ft, resourcePath("SourceCodePro-Regular.ttf"), 0, &face);

    logDefault(@"Renderer", @"%s", resourcePath("SourceCodePro-Regular.ttf"));

    FT_Set_Pixel_Sizes(face, 0, 48);

    glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height);

    return true;
}
