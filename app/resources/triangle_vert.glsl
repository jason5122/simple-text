#version 330 core

layout(location = 0) in vec2 pixelPos;

uniform vec2 viewportSize;

vec2 normalize(vec2 point) {
    vec2 inverseViewportSize = 1 / (viewportSize / 2.0);
    float clipX = (point.x * inverseViewportSize.x) - 1.0f;
    float clipY = (point.y * inverseViewportSize.y) - 1.0f;
    return vec2(clipX, clipY);
}

void main() {
    // Get the vertex coordinates in 0.0 - 1.0 range by dividing the position in pixelspace by the
    // resolution in pixels
    vec2 vertexCoords = pixelPos;

    // Uncomment this if you want origin on top left instead of bottom left
    // vertexCoords.y = 1.0 - vertexCoords.y;

    // Calculate the vertex position in -1.0 - 1.0 range by multiplying the coordinates by 2.0, and
    // then subtracting 1.0
    vec2 vertexPosition = (vertexCoords * 2.0) - 1.0;

    gl_Position.xy = vertexPosition;
    gl_Position.zw = vec2(0.0, 1.0);
}
