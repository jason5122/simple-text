#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;
flat in vec4 background_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;
uniform int rendering_pass;

float roundedRectangle(vec2 pos, vec2 size, float radius, float thickness) {
    float d = length(max(abs(tex_coords - pos), size) - size) - radius;
    return smoothstep(0.66, 0.33, d / thickness * 5.0);
}

void main() {
    if (rendering_pass == 0) {
        if (background_color.a == 0.0) discard;

        alpha_mask = vec4(1.0);
        // color = vec4(background_color.rgb * background_color.a, background_color.a);
        color = vec4(0.89, 0.902, 0.91, background_color.a);
    }

    if (rendering_pass == 1) {
        vec4 texel = texture(mask, tex_coords);

        vec2 pos = vec2(0.5, 0.5);
        vec2 size = vec2(0.16, 0.02);
        float intensity = 0.6 * roundedRectangle(pos, size, 0.1, 0.2);
        texel = mix(texel, vec4(0.2, 0.8, 0.5, 1.0), intensity);

        int colored = int(text_color.a);
        if (colored == 1) {
            // Revert alpha premultiplication.
            if (texel.a != 0.0) {
                texel.rgb /= texel.a;
            }

            alpha_mask = vec4(texel.a);
            color = vec4(texel.rgb, 1.0);
        } else {
            alpha_mask = vec4(texel.rgb, texel.r);
            color = vec4(text_color.rgb, 1.0);
        }
    }
}
