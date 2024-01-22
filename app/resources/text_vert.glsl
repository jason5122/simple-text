#version 330 core

layout(location = 0) in vec2 grid_coords;
layout(location = 1) in vec4 glyph;
layout(location = 2) in vec4 uv;
layout(location = 3) in vec4 in_text_color;  // The `colored` flag is packed along with text color.

out vec2 tex_coords;
flat out vec4 text_color;

uniform vec2 resolution;
uniform vec2 cell_dim;
uniform vec2 scroll_offset;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 glyph_offset = glyph.xy;  // <left, top>
    vec2 glyph_size = glyph.zw;    // <width, height>
    vec2 uv_offset = uv.xy;        // <uv_left, uv_bot>
    vec2 uv_size = uv.zw;          // <uv_width, uv_height>

    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    // Position of cell from top-left.
    vec2 cell_position = cell_dim * grid_coords;
    glyph_offset.y = cell_dim.y - glyph_offset.y;

    cell_position += glyph_offset + glyph_size * position;
    cell_position += scroll_offset;

    gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
    tex_coords = uv_offset + uv_size * position;
    text_color = vec4(in_text_color.rgb / 255.0, in_text_color.a);
}
