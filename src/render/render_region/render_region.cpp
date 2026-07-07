// clause: machinery — the GL boundary. The one file that speaks EGL/GLES
// (ADR-015: the region owns the device; nodes stay passive). Headless
// surfaceless EGL + an FBO; the `flat` program draws a mesh's NDC triangles
// in a surface's colour; readback is flipped to top-down so pixel rows read
// y-down (matching the golden-frame analytic expectations).
#include "render_region.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "svalue_accessors.hpp"

namespace syg::render {
namespace {

const char* kVert =
    "#version 300 es\n"
    "layout(location=0) in vec2 aPos;\n"
    "void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }\n";
const char* kFrag =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec4 uColor;\n"
    "out vec4 frag;\n"
    "void main(){ frag = uColor; }\n";

unsigned compile(GLenum type, const char* src) {
  unsigned sh = glCreateShader(type);
  glShaderSource(sh, 1, &src, nullptr);
  glCompileShader(sh);
  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(sh, 512, nullptr, log);
    throw std::runtime_error(std::string("render_region shader: ") + log);
  }
  return sh;
}

}  // namespace

render_region::~render_region() {
  if (dpy_) {
    eglMakeCurrent(static_cast<EGLDisplay>(dpy_), EGL_NO_SURFACE,
                   EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (ctx_)
      eglDestroyContext(static_cast<EGLDisplay>(dpy_),
                        static_cast<EGLContext>(ctx_));
    eglTerminate(static_cast<EGLDisplay>(dpy_));
  }
}

void render_region::ensure_gl() {
  if (gl_ready_) return;
  EGLDisplay dpy = EGL_NO_DISPLAY;
  auto get = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
      eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (get)
    dpy = get(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
  if (dpy == EGL_NO_DISPLAY) dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint maj = 0, min = 0;
  if (!eglInitialize(dpy, &maj, &min))
    throw std::runtime_error("render_region: eglInitialize failed");
  eglBindAPI(EGL_OPENGL_ES_API);
  const EGLint cfga[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                         EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                         EGL_ALPHA_SIZE, 8, EGL_NONE};
  EGLConfig cfg;
  EGLint n = 0;
  eglChooseConfig(dpy, cfga, &cfg, 1, &n);
  if (n < 1) throw std::runtime_error("render_region: no EGL config");
  const EGLint ctxa[] = {EGL_CONTEXT_MAJOR_VERSION, 3,
                         EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa);
  if (ctx == EGL_NO_CONTEXT)
    throw std::runtime_error("render_region: eglCreateContext failed");
  if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
    const EGLint pb[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
    EGLSurface s = eglCreatePbufferSurface(dpy, cfg, pb);
    if (!eglMakeCurrent(dpy, s, s, ctx))
      throw std::runtime_error("render_region: eglMakeCurrent failed");
  }
  dpy_ = dpy;
  ctx_ = ctx;

  unsigned prog = glCreateProgram();
  glAttachShader(prog, compile(GL_VERTEX_SHADER, kVert));
  glAttachShader(prog, compile(GL_FRAGMENT_SHADER, kFrag));
  glLinkProgram(prog);
  GLint linked = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &linked);
  if (!linked) throw std::runtime_error("render_region: program link failed");
  prog_ = prog;
  ucolor_ = glGetUniformLocation(prog, "uColor");

  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  glGenTextures(1, &tex_);
  glBindTexture(GL_TEXTURE_2D, tex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_, h_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex_, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("render_region: FBO incomplete");
  gl_ready_ = true;
}

void render_region::set_size(int w, int h) {
  if (w > 0) w_ = w;
  if (h > 0) h_ = h;
  if (gl_ready_) {
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w_, h_, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
  }
}

void render_region::begin_frame() {
  ensure_gl();
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);  // never FBO 0 (headless)
  glViewport(0, 0, w_, h_);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // the background == golden clear
  glClear(GL_COLOR_BUFFER_BIT);
}

void render_region::draw_one(const syg::crown::svalue& mesh,
                             const syg::crown::svalue& surface) {
  if (!mesh.value || !surface.value) return;
  const auto& md = syg::generated::as_mesh(mesh);
  const auto& sd = syg::generated::as_surface(surface);
  if (md.verts.size() < 6) return;  // need at least one triangle
  glUseProgram(prog_);
  glUniform4f(ucolor_, sd.r, sd.g, sd.b, sd.a);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(md.verts.size() * sizeof(float)),
               md.verts.data(), GL_STREAM_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(md.verts.size() / 2));
}

void render_region::end_frame() {
  glFinish();
  std::vector<unsigned char> raw(static_cast<std::size_t>(w_) * h_ * 4);
  glReadPixels(0, 0, w_, h_, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());
  // glReadPixels is bottom-up; flip to top-down so row 0 is the top of the
  // y-down image the golden-frame properties assume.
  pixels_.resize(raw.size());
  const std::size_t row = static_cast<std::size_t>(w_) * 4;
  for (int r = 0; r < h_; ++r)
    std::memcpy(&pixels_[r * row], &raw[(h_ - 1 - r) * row], row);
  ++frames_;
}

}  // namespace syg::render
