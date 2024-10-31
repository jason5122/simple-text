R"(

#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;
uniform float u_time;
uniform vec2 resolution;

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

void main() {
    vec4 texel = texture(mask, tex_coords);

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
        // color = vec4(text_color.rgb, 1.0);

        // https://github.com/tsoding/ded/blob/ea30e9d6ee1c0d52aa11f9386920b884987a6b55/shaders/simple_epic.frag
        vec2 frag_uv = gl_FragCoord.xy / resolution;
        vec3 rainbow = hsl2rgb(vec3((u_time + frag_uv.x + frag_uv.y), 0.5, 0.5));
        color = vec4(rainbow, 1.0);
    }
}

)"
