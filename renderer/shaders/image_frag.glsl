#version 330 core

in vec2 tex_coords;
flat in vec3 image_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    alpha_mask = vec4(1.0);
    color = vec4(image_color, texel.a);
}
