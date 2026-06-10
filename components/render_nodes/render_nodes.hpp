// Copyright 2026 Travis West
#pragma once
#include "gpu_texture.hpp"
#include "gl_program.hpp"
#include <memory>
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <string_view>

// Offscreen render graph plumbing. render_target consumes a draw-call edge
// (the producer then skips the global pass) and emits the scene as a
// texture; texture_view puts a texture on screen. Chains compose:
// draw → render_target → glsl_effect... → texture_view.

struct RenderTargetNode {
    static consteval std::string_view name() { return "render_target"; }
    static consteval std::string_view source_header() { return "components/render_nodes/render_nodes.hpp"; }
    struct inputs {
        port<"draw", DrawFn>        draw;
        port<"pv",   Eigen::Matrix4f> pv;
        slider<"width",  "", float, fp(64.f), fp(2048.f), fp(512.f)> width;
        slider<"height", "", float, fp(64.f), fp(2048.f), fp(512.f)> height;
        slider<"bg_r", "", float, fp(0.f), fp(1.f), fp(0.05f)> bg_r;
        slider<"bg_g", "", float, fp(0.f), fp(1.f), fp(0.05f)> bg_g;
        slider<"bg_b", "", float, fp(0.f), fp(1.f), fp(0.1f)>  bg_b;
    } inputs;
    struct outputs {
        port<"texture", GpuTexture> texture;
    } outputs;
    RenderTargetNode() { inputs.pv.value.setIdentity(); }
    void operator()(double);
    ~RenderTargetNode();
private:
    GLuint fbo_ = 0, color_ = 0, depth_ = 0;
    int    w_ = 0, h_ = 0;
};

struct TextureViewNode {
    static consteval std::string_view name() { return "texture_view"; }
    static consteval std::string_view source_header() { return "components/render_nodes/render_nodes.hpp"; }
    struct inputs {
        port<"texture", GpuTexture> texture;
        slider<"alpha", "", float, fp(0.f), fp(1.f), fp(1.f)> alpha;
    } inputs;
    struct outputs {
        port<"render", DrawFn> render;
    } outputs;
    void operator()(double);
private:
    GlProgram* prog();  // lazy; render thread only
    GLuint vao_ = 0;
    GpuTexture tex_{};
    std::unique_ptr<GlProgram> prog_;
};
