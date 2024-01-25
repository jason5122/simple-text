#version 330 core

out vec4 frag_color;

void main() {
    vec3 blue2 = vec3(95, 180, 180) / 255.0;

    frag_color = vec4(blue2, 1.0);
}
