#version 330 core

in vec2 TexCoords;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alphaMask;

uniform sampler2D mask;

void main() {
    vec3 textColor = texture(mask, TexCoords).rgb;
    alphaMask = vec4(textColor, textColor.r);
    color = vec4(51 / 255.0, 51 / 255.0, 51 / 255.0, 1.0);
}
