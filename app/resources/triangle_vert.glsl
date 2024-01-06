#version 330 core

layout(location = 0) in vec3 pixelPos;

uniform vec2 viewportSize;

void main() {
    gl_Position = vec4(pixelPos, 1.0);
}
