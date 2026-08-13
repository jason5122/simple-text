R"(

#version 330 core

// Instanced glyph quads: no per-vertex data. The four corners come from gl_VertexID; each glyph is
// one instance carrying its device-pixel rect, atlas uv rect, tint color, and a flag. u_proj maps
// device pixels (top-left origin) to NDC, so scroll/zoom/tilt all live in that matrix.

layout(location = 0) in vec4 i_rect;    // x, y, w, h in device pixels (top-left origin)
layout(location = 1) in vec4 i_uv;      // u, v, uw, vh in the atlas
layout(location = 2) in vec4 i_color;   // tint for mono glyphs; ignored for color/debug
layout(location = 3) in float i_flags;  // 0 = mono glyph, 1 = color glyph, 2 = atlas debug

uniform mat4 u_proj;

out vec2 v_uv;
flat out vec4 v_color;
flat out float v_flags;

void main() {
    vec2 corner = vec2(gl_VertexID & 1, (gl_VertexID >> 1) & 1);  // (0,0) (1,0) (0,1) (1,1)
    vec2 pos = i_rect.xy + corner * i_rect.zw;
    gl_Position = u_proj * vec4(pos, 0.0, 1.0);
    v_uv = i_uv.xy + corner * i_uv.zw;
    v_color = i_color;
    v_flags = i_flags;
}

)"
