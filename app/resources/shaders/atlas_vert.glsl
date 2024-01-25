#version 330 core

layout(location = 0) in vec4 coords;

out vec2 tex_coords;

uniform vec2 resolution;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    gl_Position = vec4(pixelToClipSpace(coords.xy), 0.0, 1.0);
    tex_coords = coords.zw;
}