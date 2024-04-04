R"(

#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 rect_size;
layout(location = 2) in vec4 uv;
layout(location = 3) in vec3 in_color;

out vec2 tex_coords;
flat out vec3 image_color;

uniform vec2 resolution;
uniform vec2 scroll_offset;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 final_position = coords + rect_size * position;
    // final_position += scroll_offset;

    vec2 uv_offset = uv.xy;  // <uv_left, uv_bot>
    vec2 uv_size = uv.zw;    // <uv_width, uv_height>

    gl_Position = vec4(pixelToClipSpace(final_position), 0.0, 1.0);
    tex_coords = uv_offset + uv_size * position;
    image_color = in_color / 255.0;
}

)"
