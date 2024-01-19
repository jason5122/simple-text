#version 330 core

in vec2 TexCoords;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alphaMask;

uniform sampler2D mask;

void main() {
    vec3 black = vec3(51, 51, 51) / 255.0;
    vec3 yellow = vec3(249, 174, 88) / 255.0;
    vec3 blue = vec3(102, 153, 204) / 255.0;

    vec3 textColor = texture(mask, TexCoords).rgb;
    alphaMask = vec4(textColor, textColor.r);
    color = vec4(yellow, 1.0);
}
