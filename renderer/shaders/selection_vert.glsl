R"(

#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec2 in_bg_size;
layout(location = 2) in vec4 in_bg_color;
// The `border_flags` flag is packed along with background border color.
layout(location = 3) in vec4 in_bg_border_color;
layout(location = 4) in int in_border_flags;

flat out vec2 bg_center;
flat out vec2 bg_size;
flat out vec4 bg_color;
flat out vec4 bg_border_color;
flat out int border_flags;

uniform vec2 resolution;
uniform vec2 scroll_offset;
uniform vec2 editor_offset;
uniform int corner_radius;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 bg_size_temp = in_bg_size;
    bg_size_temp.x += corner_radius * 2;

    vec2 cell_position = coords;
    cell_position -= scroll_offset;
    cell_position += editor_offset;

    cell_position += bg_size_temp * position;

    cell_position.x -= corner_radius;

    gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);

    bg_center = cell_position + bg_size_temp / 2;
    // Set origin at top left instead of bottom left.
    bg_center.y = resolution.y - bg_center.y;
    bg_size = bg_size_temp;
    bg_color = in_bg_color / 255.0;
    bg_border_color = vec4(in_bg_border_color.rgb / 255.0, in_bg_border_color.a);
    border_flags = in_border_flags;
}

)"
