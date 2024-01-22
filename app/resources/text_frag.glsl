#version 330 core

in vec2 tex_coords;
flat in int colored;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    if (colored == 1) {
        // Revert alpha premultiplication.
        if (texel.a != 0.0) {
            texel.rgb /= texel.a;
        }

        alpha_mask = vec4(texel.a);
        color = vec4(texel.rgb, 1.0);
    } else {
        vec3 black = vec3(51, 51, 51) / 255.0;
        vec3 yellow = vec3(249, 174, 88) / 255.0;
        vec3 blue = vec3(102, 153, 204) / 255.0;

        alpha_mask = vec4(texel.rgb, texel.r);
        color = vec4(black, 1.0);
    }
}
