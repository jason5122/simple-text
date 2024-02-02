#version 330 core

layout(location = 0) in vec2 grid_coords;
layout(location = 1) in vec4 glyph;
layout(location = 2) in vec4 uv;
layout(location = 3) in vec4 in_text_color;  // The `colored` flag is packed along with text color.
layout(location = 4) in float total_advance;
layout(location = 5) in vec4 in_background_color;
layout(location = 6) in float advance;

out vec2 tex_coords;
flat out vec4 text_color;
flat out vec4 background_color;

uniform vec2 resolution;
uniform float line_height;
uniform vec2 scroll_offset;
uniform int rendering_pass;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    vec2 cell_position = vec2(total_advance, line_height * grid_coords.y);
    cell_position += scroll_offset;

    if (rendering_pass == 0) {
        cell_position.x += advance * position.x;
        cell_position.y += line_height * position.y;

        gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
        background_color = in_background_color / 255.0;
    } else {
        vec2 glyph_offset = glyph.xy;  // <left, top>
        vec2 glyph_size = glyph.zw;    // <width, height>
        vec2 uv_offset = uv.xy;        // <uv_left, uv_bot>
        vec2 uv_size = uv.zw;          // <uv_width, uv_height>

        glyph_offset.y = line_height - glyph_offset.y;
        cell_position += glyph_offset + glyph_size * position;

        gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
        tex_coords = uv_offset + uv_size * position;
        text_color = vec4(in_text_color.rgb / 255.0, in_text_color.a);
    }
}
