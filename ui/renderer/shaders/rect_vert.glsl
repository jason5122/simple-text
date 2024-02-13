#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 rect_size;
layout(location = 2) in vec4 in_color;

flat out vec4 color;

out vec2 upper_left;
out vec2 size;

uniform vec2 resolution;
uniform vec2 scroll_offset;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 final_position = coords + rect_size * position;
    // final_position -= scroll_offset;
    final_position.x += 400;
    final_position.y += 60;

    gl_Position = vec4(pixelToClipSpace(final_position), 0.0, 1.0);
    color = in_color / 255.0;

    upper_left = final_position;
    size = vec2(0, 0);
}
