#version 330 core

flat in vec4 color;

out vec4 frag_color;

in vec2 upper_left;
in vec2 size;

uniform vec2 resolution;

void main() {
    vec2 loc = gl_FragCoord.xy / resolution;  // Normalize to [0.0, 1.0].

    // if (loc.x < 0.5 && loc.y < 0.5) {
    //     discard;
    // }

    // if (loc.x < 0.5 && loc.y < upper_left.y) {
    //     discard;
    // }

    // vec2 upper_left_scaled = upper_left / resolution;
    // if (upper_left_scaled.y < 0.5) {
    //     discard;
    // }

    float x = 0.125;
    float y = 0.4;
    // if (distance(loc.x, x) < 0.1 && distance(loc.y, y) < 0.1) {
    //     discard;
    // }

    vec2 r0 = vec2(x, y);
    if (distance(loc, r0) < 0.1) {
        discard;
    }

    frag_color = color;
}
