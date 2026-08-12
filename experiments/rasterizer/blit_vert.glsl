R"(

#version 330 core

// Attributeless textured quad: 4 vertices drawn as a triangle strip, corners derived from
// gl_VertexID (no VBO, no attributes). u_extent is the quad's size in NDC, which the CPU computes
// once as 2 * texture_px / framebuffer_px -- so the texture lands 1:1 at the top-left with no
// per-vertex divide and no projection matrix. The texture data is top-down, so v = corner.y.

uniform vec2 u_extent;

out vec2 v_uv;

void main() {
    vec2 corner = vec2(gl_VertexID & 1, (gl_VertexID >> 1) & 1);  // (0,0) (1,0) (0,1) (1,1)
    v_uv = corner;
    gl_Position = vec4(corner.x * u_extent.x - 1.0, 1.0 - corner.y * u_extent.y, 0.0, 1.0);
}

)"
