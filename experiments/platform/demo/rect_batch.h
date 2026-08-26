// A small rectangle stream shaped like Sublime Text's dynamic GL streams.
//
// Each stream owns two RGBA32F texture-buffer slots. A slot's storage is allocated only when it
// must grow; ordinary frames update the existing storage with glBufferSubData. Rectangles are then
// consumed as instances of a static four-vertex strip. This is the same GL object topology and draw
// shape used by ST's rectangle stream.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "experiments/platform/px.h"
#include "experiments/platform/px_gl.h"

namespace px_demo {

namespace rect_batch_detail {

inline constexpr const char* kVertexShader = R"(#version 150
out vec4 v_color;
uniform vec2 u_viewport;
uniform samplerBuffer u_instances;
void main() {
  vec4 geometry = texelFetch(u_instances, gl_InstanceID * 2);
  v_color = texelFetch(u_instances, gl_InstanceID * 2 + 1);
  vec2 corner;
  corner.x = (gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : 0.0;
  corner.y = (gl_VertexID >= 2) ? 1.0 : 0.0;
  vec2 pos = geometry.xy + corner * geometry.zw;
  vec2 ndc = vec2(pos.x / u_viewport.x * 2.0 - 1.0,
                  1.0 - pos.y / u_viewport.y * 2.0);
  gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

inline constexpr const char* kFragmentShader = R"(#version 150
in vec4 v_color;
out vec4 frag_color;
void main() { frag_color = v_color; }
)";

inline GLuint compile_shader(GLenum type, const char* source) {
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024] = {};
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::fprintf(stderr, "shader compile failed: %s\n", log);
  }
  return shader;
}

struct Instance {
  float x;
  float y;
  float width;
  float height;
  float r;
  float g;
  float b;
  float a;
};

}  // namespace rect_batch_detail

class RectBatch {
 public:
  bool usable() const { return program_ != 0; }

  void ensure_initialized() {
    if (program_ || !px_gl_has_shaders()) {
      return;
    }

    const GLuint vs =
        rect_batch_detail::compile_shader(GL_VERTEX_SHADER, rect_batch_detail::kVertexShader);
    const GLuint fs =
        rect_batch_detail::compile_shader(GL_FRAGMENT_SHADER, rect_batch_detail::kFragmentShader);
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    viewport_uniform_ = glGetUniformLocation(program_, "u_viewport");
    instances_uniform_ = glGetUniformLocation(program_, "u_instances");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(2, buffers_);
    glGenTextures(2, textures_);
    glBindVertexArray(0);
  }

  void add(rect r, fcolor c) {
    instances_.push_back(rect_batch_detail::Instance{
        static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
        static_cast<float>(r.h), c.r, c.g, c.b, c.a});
  }

  void flush(vec2 viewport_points) {
    if (instances_.empty() || !usable()) {
      instances_.clear();
      return;
    }

    // ST toggles the selected stream slot before binding and uploading it.
    slot_ ^= 1;
    const size_t required = instances_.size() * sizeof(rect_batch_detail::Instance);

    glUseProgram(program_);
    glUniform2f(viewport_uniform_, static_cast<float>(viewport_points.x),
                static_cast<float>(viewport_points.y));
    glUniform1i(instances_uniform_, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_TEXTURE_BUFFER, buffers_[slot_]);
    if (capacities_[slot_] < required) {
      capacities_[slot_] = std::max(required, capacities_[slot_] * 2);
      glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizeiptr>(capacities_[slot_]), nullptr,
                   GL_STREAM_DRAW);
      glBindTexture(GL_TEXTURE_BUFFER, textures_[slot_]);
      glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers_[slot_]);
    }
    glBufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizeiptr>(required), instances_.data());
    glBindTexture(GL_TEXTURE_BUFFER, textures_[slot_]);
    glBindVertexArray(vao_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances_.size()));
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    instances_.clear();
  }

 private:
  GLuint program_ = 0;
  GLuint vao_ = 0;
  GLuint buffers_[2] = {};
  GLuint textures_[2] = {};
  size_t capacities_[2] = {};
  int slot_ = 0;
  GLint viewport_uniform_ = -1;
  GLint instances_uniform_ = -1;
  std::vector<rect_batch_detail::Instance> instances_;
};

}  // namespace px_demo
