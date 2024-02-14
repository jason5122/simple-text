#version 330 core

flat in vec4 color;

out vec4 frag_color;

flat in vec2 center;
flat in vec2 size;

uniform vec2 resolution;

void main() {
    float corner_radius = 200;

    vec2 loc = gl_FragCoord.xy;

    vec2 top_right = center + size / 2;
    top_right -= corner_radius;
    if (loc.x > top_right.x && loc.y > top_right.y) {
        if (distance(loc, top_right) > corner_radius) {
            discard;
        }
    }

    frag_color = color;
}
