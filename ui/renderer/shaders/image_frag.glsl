#version 330 core

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

void main() {
    alpha_mask = vec4(1.0);
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
