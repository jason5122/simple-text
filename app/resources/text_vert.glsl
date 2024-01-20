#version 330 core

layout(location = 0) in vec4 vertex;  // <vec2 pixel_pos, vec2 tex_coords>
layout(location = 1) in vec2 grid_coords;

out vec2 tex_coords;

uniform vec2 resolution;
uniform vec2 cell_dim;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    gl_Position = vec4(pixelToClipSpace(vertex.xy + grid_coords * cell_dim), 0.0, 1.0);
    tex_coords = vertex.zw;
}
