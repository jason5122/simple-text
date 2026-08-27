R"(
#version 150

// Instanced solid-rectangle vertex shader.

out vec4 v_color;
uniform vec2 viewport;
uniform samplerBuffer instances;

void main() {
  vec4 geometry = texelFetch(instances, gl_InstanceID * 2);
  v_color = texelFetch(instances, gl_InstanceID * 2 + 1);
  vec2 corner;
  corner.x = (gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : 0.0;
  corner.y = (gl_VertexID >= 2) ? 1.0 : 0.0;
  vec2 pos = geometry.xy + corner * geometry.zw;
  gl_Position = vec4(pos.x / viewport.x * 2.0 - 1.0,
                     1.0 - pos.y / viewport.y * 2.0, 0.0, 1.0);
}
)"
