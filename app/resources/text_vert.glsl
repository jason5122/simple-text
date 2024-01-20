#version 330 core

layout(location = 0) in vec2 grid_coords;
layout(location = 1) in vec4 glyph;
layout(location = 2) in vec4 uv;

out vec2 tex_coords;

uniform vec2 resolution;
uniform vec2 cell_dim;
uniform vec2 scroll_offset;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
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

    vec2 cell_position = cell_dim * grid_coords;
    cell_position += glyph_offset + glyph_size * position;

    cell_position += scroll_offset;

    gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
    tex_coords = uv_offset + uv_size * position;
}
