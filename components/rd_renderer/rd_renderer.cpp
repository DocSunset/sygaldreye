// Copyright 2025 Travis West
#include "rd_renderer.hpp"
#include <utility>

static constexpr const char* kVert = R"glsl(
#version 300 es
void main() {
    vec2 pos = vec2(float((gl_VertexID & 1) * 2 - 1),
                    float((gl_VertexID >> 1) * 2 - 1));
    gl_Position = vec4(pos, 0.0, 1.0);
}
)glsl";

static constexpr const char* kFrag = R"glsl(
#version 300 es
precision mediump float;
uniform sampler2D uTex;
uniform vec3 uColorA;
uniform vec3 uColorB;
out vec4 fragColor;
void main() {
    ivec2 sz = textureSize(uTex, 0);
    vec2 uv = gl_FragCoord.xy / vec2(sz);
    float v = texture(uTex, uv).g;
    fragColor = vec4(mix(uColorA, uColorB, v), 1.0);
}
)glsl";

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

static GLuint link_program(const char* vert_src, const char* frag_src) {
    GLuint vs = compile(GL_VERTEX_SHADER,   vert_src);
    GLuint fs = compile(GL_FRAGMENT_SHADER, frag_src);
    GLuint p  = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDetachShader(p, vs);
    glDetachShader(p, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

RDRenderer RDRenderer::create() {
    RDRenderer r{RawTag{}};
    r.prog_        = link_program(kVert, kFrag);
    r.tex_loc_     = glGetUniformLocation(r.prog_, "uTex");
    r.color_a_loc_ = glGetUniformLocation(r.prog_, "uColorA");
    r.color_b_loc_ = glGetUniformLocation(r.prog_, "uColorB");
    glGenVertexArrays(1, &r.vao_);
    return r;
}

RDRenderer::~RDRenderer() {
    if (prog_ != 0) { glDeleteProgram(prog_); }
    if (vao_  != 0) { glDeleteVertexArrays(1, &vao_); }
}

RDRenderer::RDRenderer(RDRenderer&& o) noexcept
    : inputs(o.inputs), outputs(o.outputs),
      prog_(std::exchange(o.prog_, 0)),
      vao_ (std::exchange(o.vao_,  0)),
      tex_loc_    (o.tex_loc_),
      color_a_loc_(o.color_a_loc_),
      color_b_loc_(o.color_b_loc_),
      texture_(o.texture_),
      color_a_(o.color_a_),
      color_b_(o.color_b_) {}

RDRenderer& RDRenderer::operator=(RDRenderer&& o) noexcept {
    if (this != &o) {
        if (prog_ != 0) { glDeleteProgram(prog_); }
        if (vao_  != 0) { glDeleteVertexArrays(1, &vao_); }
        inputs  = o.inputs;
        outputs = o.outputs;
        prog_        = std::exchange(o.prog_, 0);
        vao_         = std::exchange(o.vao_,  0);
        tex_loc_     = o.tex_loc_;
        color_a_loc_ = o.color_a_loc_;
        color_b_loc_ = o.color_b_loc_;
        texture_     = o.texture_;
        color_a_     = o.color_a_;
        color_b_     = o.color_b_;
    }
    return *this;
}

void RDRenderer::operator()(double) {
    if (!prog_) {  // graph nodes are default-constructed; create() never ran
        auto saved = inputs;
        *this = create();
        inputs = saved;
    }
    texture_ = inputs.texture.value;
    color_a_ = {inputs.r_a.value, inputs.g_a.value, inputs.b_a.value};
    color_b_ = {inputs.r_b.value, inputs.g_b.value, inputs.b_b.value};
    outputs.render.value = [this](const Eigen::Matrix4f&) { draw({}); };
}

void RDRenderer::draw(Eigen::Matrix4f const&) const {
    if (!texture_.valid() || prog_ == 0) { return; }
    glUseProgram(prog_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_.id);
    glUniform1i(tex_loc_, 0);
    glUniform3fv(color_a_loc_, 1, color_a_.data());
    glUniform3fv(color_b_loc_, 1, color_b_.data());
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
