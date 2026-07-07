// clause: machinery — the GL boundary (the render package's frame executor,
// the graphics analogue of the audio dac). The ONLY files that speak EGL/GL
// are those whose basename contains "render_region" (pkg41 exempts exactly
// that). GL handles are stored as their underlying scalar types so this
// header names no GL and pulls in no GL headers.
#pragma once

#include <vector>

#include "executor/render_target.hpp"

namespace syg::render {

// A headless GLES3 render target: surfaceless EGL context, one built-in
// `flat` program, an FBO sized to the requested frame, RGBA readback. GL
// objects are acquired on the first frame (L9 — never in the constructor),
// loudly on failure.
class render_region : public syg::executor::render_target {
 public:
  render_region() = default;
  ~render_region() override;

  void set_size(int w, int h) override;
  void begin_frame() override;   // bind FBO, clear to the background
  void draw_one(const syg::crown::svalue& mesh,
                const syg::crown::svalue& surface) override;
  void end_frame() override;     // finish + read back (top-down) + count
  int frames_rendered() const override { return frames_; }
  const std::vector<unsigned char>& rgba() const override { return pixels_; }

 private:
  void ensure_gl();  // first-frame acquisition; throws on EGL/GL failure

  // EGL handles (void* = EGLDisplay/EGLContext) and GL names (GLuint =
  // unsigned) kept type-erased so the header stays GL-free.
  void* dpy_ = nullptr;
  void* ctx_ = nullptr;
  unsigned prog_ = 0, vao_ = 0, vbo_ = 0, fbo_ = 0, tex_ = 0;
  int ucolor_ = -1;
  bool gl_ready_ = false;
  int w_ = 128, h_ = 128, frames_ = 0;
  std::vector<unsigned char> pixels_;  // top-down RGBA8, w_*h_*4
};

}  // namespace syg::render
