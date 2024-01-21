#version 330 core

in vec2 tex_coords;
flat in int colored;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    if (colored == 1) {
        color = texture(mask, tex_coords);
        alpha_mask = vec4(color.a);

        // Revert alpha premultiplication.
        if (color.a != 0.0) {
            color.rgb = vec3(color.rgb / color.a);
        }

        color = vec4(color.rgb, 1.0);
    } else {
        vec3 black = vec3(51, 51, 51) / 255.0;
        vec3 yellow = vec3(249, 174, 88) / 255.0;
        vec3 blue = vec3(102, 153, 204) / 255.0;

        vec3 text_color = texture(mask, tex_coords).rgb;
        alpha_mask = vec4(text_color, text_color.r);
        color = vec4(black, 1.0);
    }
}
