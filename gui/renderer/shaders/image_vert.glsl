R"(

#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 rect_size;
layout(location = 2) in vec4 uv;
// The `colored` flag is packed along with color.
layout(location = 3) in vec4 in_image_color;

out vec2 tex_coords;
flat out vec4 image_color;

uniform vec2 resolution;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin at top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 final_position = coords + rect_size * position;

    vec2 uv_offset = uv.xy;
    vec2 uv_size = uv.zw;

    gl_Position = vec4(pixelToClipSpace(final_position), 0.0, 1.0);
    tex_coords = uv_offset + uv_size * position;
    image_color = vec4(in_image_color.rgb / 255.0, in_image_color.a);
}

)"
