#version 330 core

layout(location = 0) in vec4 vertex;  // <vec2 pos, vec2 tex>
layout(location = 1) in vec2 offset;

out vec2 TexCoords;

uniform vec2 resolution;

vec2 pixelToClipSpace(vec2 point) {
    point /= resolution;         // Normalize to [0.0, 1.0].
    return (point * 2.0) - 1.0;  // Convert to [-1.0, 1.0].
}

void main() {
    gl_Position = vec4(pixelToClipSpace(vertex.xy + offset), 0.0, 1.0);
    TexCoords = vertex.zw;
}
