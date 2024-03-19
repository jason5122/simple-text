#version 330 core

layout(location = 0) in vec2 coords;
layout(location = 1) in vec4 glyph;
layout(location = 2) in vec4 uv;
layout(location = 3) in vec4 in_text_color;  // The `colored` flag is packed along with text color.
layout(location = 4) in int is_atlas;
layout(location = 5) in vec2 in_bg_size;
layout(location = 6) in vec4 in_bg_color;

out vec2 tex_coords;
flat out vec4 text_color;
flat out vec4 bg_color;
flat out vec2 bg_center;
flat out vec2 bg_size;

uniform vec2 resolution;
uniform float line_height;
uniform vec2 scroll_offset;
uniform vec2 editor_offset;
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

    vec2 cell_position = coords;
    cell_position -= scroll_offset;
    cell_position += editor_offset;

    if (rendering_pass == 0) {
        cell_position += in_bg_size * position;

        gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
        bg_color = in_bg_color / 255.0;

        bg_center = cell_position + in_bg_size / 2;
        // Set origin at top left instead of bottom left.
        bg_center.y = resolution.y - bg_center.y;
        bg_size = in_bg_size;
    }

    if (rendering_pass == 1) {
        vec2 glyph_offset = glyph.xy;  // <left, top>
        vec2 glyph_size = glyph.zw;    // <width, height>
        vec2 uv_offset = uv.xy;        // <uv_left, uv_bot>
        vec2 uv_size = uv.zw;          // <uv_width, uv_height>

        if (is_atlas == 0) {
            glyph_offset.y = line_height - glyph_offset.y;
        }
        cell_position += glyph_offset + glyph_size * position;

        gl_Position = vec4(pixelToClipSpace(cell_position), 0.0, 1.0);
        tex_coords = uv_offset + uv_size * position;
        text_color = vec4(in_text_color.rgb / 255.0, in_text_color.a);
    }
}
