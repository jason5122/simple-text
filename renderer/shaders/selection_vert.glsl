R"(

#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 in_size;
layout(location = 2) in vec4 in_color;
// The `border_flags` flag is packed along with background border color.
layout(location = 3) in vec4 in_border_color;
layout(location = 4) in ivec4 border_info;

flat out vec2 center;
flat out vec2 size;
flat out vec4 color;
flat out vec4 border_color;
flat out int border_flags;
flat out int bottom_border_offset;
flat out int top_border_offset;
flat out int hide_background;

uniform vec2 resolution;
uniform vec2 scroll_offset;
uniform vec2 editor_offset;
uniform float line_number_offset;
uniform int r;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 size_temp = in_size;
    size_temp.x += r * 2;

    vec2 cell_position = coords;
    cell_position -= scroll_offset;
    cell_position += editor_offset;
    cell_position.x += line_number_offset;

    cell_position += size_temp * position;

    cell_position.x -= r;

    gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);

    center = cell_position + size_temp / 2;
    // Set origin at top left instead of bottom left.
    center.y = resolution.y - center.y;
    size = size_temp;
    color = in_color / 255.0;
    border_color = in_border_color / 255.0;
    border_flags = border_info.x;
    bottom_border_offset = border_info.y;
    top_border_offset = border_info.z;
    hide_background = border_info.w;
}

)"
