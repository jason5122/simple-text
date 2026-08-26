// The rectangle batch used by platform_demo before the ST-style texture-buffer experiment.
//
// It deliberately owns one vertex buffer and replaces its storage with glBufferData on every
// flush. Combined with the macOS layer's one-frame fence, this is the "fence-only" comparison path
// that produced the lowest measured cursor-follow latency.

#pragma once

#include <cstddef>
#include <cstdio>
#include <vector>

#include "experiments/platform/px.h"
#include "experiments/platform/px_gl.h"

namespace px_demo {

namespace fence_rect_batch_detail {

inline constexpr const char* kVertexShader = R"(#version 150
in vec2 a_pos;
in vec4 a_color;
out vec4 v_color;
uniform vec2 u_viewport;
void main() {
  vec2 ndc = vec2(a_pos.x / u_viewport.x * 2.0 - 1.0,
                  1.0 - a_pos.y / u_viewport.y * 2.0);
  gl_Position = vec4(ndc, 0.0, 1.0);
  v_color = a_color;
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

struct Vertex {
  float x;
  float y;
  float r;
  float g;
  float b;
  float a;
};

}  // namespace fence_rect_batch_detail

class FenceRectBatch {
 public:
  bool usable() const { return program_ != 0; }

  void ensure_initialized() {
    if (program_ || !px_gl_has_shaders()) {
      return;
    }

    const GLuint vs = fence_rect_batch_detail::compile_shader(
        GL_VERTEX_SHADER, fence_rect_batch_detail::kVertexShader);
    const GLuint fs = fence_rect_batch_detail::compile_shader(
        GL_FRAGMENT_SHADER, fence_rect_batch_detail::kFragmentShader);
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glBindAttribLocation(program_, 0, "a_pos");
    glBindAttribLocation(program_, 1, "a_color");
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    viewport_uniform_ = glGetUniformLocation(program_, "u_viewport");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(fence_rect_batch_detail::Vertex),
                          reinterpret_cast<void*>(offsetof(fence_rect_batch_detail::Vertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(fence_rect_batch_detail::Vertex),
                          reinterpret_cast<void*>(offsetof(fence_rect_batch_detail::Vertex, r)));
    glBindVertexArray(0);
  }

  void add(rect r, fcolor c) {
    using fence_rect_batch_detail::Vertex;
    const Vertex tl{static_cast<float>(r.x), static_cast<float>(r.y), c.r, c.g, c.b, c.a};
    const Vertex tr{static_cast<float>(r.right()), static_cast<float>(r.y), c.r, c.g, c.b, c.a};
    const Vertex br{static_cast<float>(r.right()), static_cast<float>(r.bottom()),
                    c.r, c.g, c.b, c.a};
    const Vertex bl{static_cast<float>(r.x), static_cast<float>(r.bottom()), c.r, c.g, c.b, c.a};
    vertices_.insert(vertices_.end(), {tl, tr, br, tl, br, bl});
  }

  void flush(vec2 viewport_points) {
    if (vertices_.empty() || !usable()) {
      vertices_.clear();
      return;
    }

    glUseProgram(program_);
    glUniform2f(viewport_uniform_, static_cast<float>(viewport_points.x),
                static_cast<float>(viewport_points.y));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices_.size() * sizeof(fence_rect_batch_detail::Vertex)),
                 vertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    glBindVertexArray(0);
    vertices_.clear();
  }

 private:
  GLuint program_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLint viewport_uniform_ = -1;
  std::vector<fence_rect_batch_detail::Vertex> vertices_;
};

}  // namespace px_demo
