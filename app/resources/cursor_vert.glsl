#version 330 core

layout(location = 0) in vec2 coords;

uniform vec2 resolution;
uniform vec2 cell_dim;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    point.y = 1.0 - point.y;     // Set origin to top left instead of bottom left.
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    // gl_Position = vec4(pixelToClipSpace(cell_dim * 10), 0.0, 1.0);
    gl_Position = vec4(pixelToClipSpace(coords), 0.0, 1.0);
}
