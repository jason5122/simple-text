R"(

#version 330 core
in vec2 aPos;

uniform vec2 uOffsetClip;
uniform vec2 uScaleClip;

void main() {
    vec2 p = aPos * uScaleClip + uOffsetClip;
    gl_Position = vec4(p, 0.0, 1.0);
}

)"
