#version 330 core

flat in vec4 color;

out vec4 frag_color;

flat in vec2 center;
flat in vec2 size;

uniform vec2 resolution;

void main() {
    float corner_radius = 10;

    vec2 loc = gl_FragCoord.xy;
    loc.y = resolution.y - loc.y;  // Set origin at top left instead of bottom left.

    vec2 bottom_left = center - size / 2;
    vec2 bottom_right = center + vec2(size.x / 2, -size.y / 2);
    vec2 top_left = center + vec2(-size.x / 2, size.y / 2);
    vec2 top_right = center + size / 2;

    bottom_left += corner_radius;
    bottom_right += vec2(-corner_radius, corner_radius);
    top_left += vec2(corner_radius, -corner_radius);
    top_right -= corner_radius;

    if (loc.x < bottom_left.x && loc.y < bottom_left.y) {
        if (distance(loc, bottom_left) > corner_radius) {
            discard;
        }
    }
    if (loc.x > bottom_right.x && loc.y < bottom_right.y) {
        if (distance(loc, bottom_right) > corner_radius) {
            discard;
        }
    }
    if (loc.x < top_left.x && loc.y > top_left.y) {
        if (distance(loc, top_left) > corner_radius) {
            discard;
        }
    }
    if (loc.x > top_right.x && loc.y > top_right.y) {
        if (distance(loc, top_right) > corner_radius) {
            discard;
        }
    }

    frag_color = color;
}
