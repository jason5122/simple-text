#version 330 core

layout(location = 0) in vec2 pixelPos;

uniform vec2 viewportSize;

void main() {
    gl_Position.xy = pixelPos;
    gl_Position.zw = vec2(0.0, 1.0);
}
