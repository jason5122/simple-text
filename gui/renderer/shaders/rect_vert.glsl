R"(

#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 rect_size;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec4 in_extra;

flat out vec4 rect_color;
flat out vec2 rect_center;
flat out vec2 size;
flat out vec4 extra;

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

    gl_Position = vec4(pixelToClipSpace(final_position), 0.0, 1.0);
    rect_color = in_color / 255.0;

    rect_center = final_position + rect_size / 2;
    // Set origin at top left instead of bottom left.
    rect_center.y = resolution.y - rect_center.y;

    size = rect_size;
    extra = in_extra;
}

)"
