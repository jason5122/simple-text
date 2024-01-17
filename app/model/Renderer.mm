#import "Renderer.h"
#import <glm/glm.hpp>
#import <glm/gtc/matrix_transform.hpp>
#import <glm/gtc/type_ptr.hpp>

Renderer::Renderer(float width, float height) {
    glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height);
}
