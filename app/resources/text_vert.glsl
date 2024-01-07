#version 330 core

// Cell properties.
layout(location = 0) in vec2 gridCoords;

// Glyph properties.
layout(location = 1) in vec4 glyph;

// uv mapping.
layout(location = 2) in vec4 uv;

out vec2 TexCoords;

// Terminal properties
uniform vec2 cellDim;
uniform vec4 projection;

void main() {
    vec2 glyphOffset = glyph.xy;
    vec2 glyphSize = glyph.zw;
    vec2 uvOffset = uv.xy;
    vec2 uvSize = uv.zw;
    vec2 projectionOffset = projection.xy;
    vec2 projectionScale = projection.zw;

    // Compute vertex corner position
    vec2 position;
    position.x = (gl_VertexID == 0 || gl_VertexID == 1) ? 1. : 0.;
    position.y = (gl_VertexID == 0 || gl_VertexID == 3) ? 0. : 1.;

    // Position of cell from top-left
    vec2 cellPosition = cellDim * gridCoords;

    glyphOffset.y = cellDim.y - glyphOffset.y;

    vec2 finalPosition = cellPosition + glyphSize * position + glyphOffset;
    gl_Position = vec4(projectionOffset + projectionScale * finalPosition, 0.0, 1.0);

    TexCoords = uvOffset + position * uvSize;
}