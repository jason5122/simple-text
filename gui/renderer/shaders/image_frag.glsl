R"(

#version 330 core

in vec2 tex_coords;
flat in vec4 image_color;

out vec4 out_color;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    int colored = int(image_color.a);
    if (colored == 1) {
        out_color = texel;
    } else {
        out_color = vec4(image_color.rgb, texel.a);
    }
}

)"
