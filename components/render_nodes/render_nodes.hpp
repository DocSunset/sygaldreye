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
    struct endpoints {
        ::in<DrawFn> draw;
        ::in<Eigen::Matrix4f> pv;
        normalled_in<float, fp(64.f), fp(2048.f), fp(512.f)> width;
        normalled_in<float, fp(64.f), fp(2048.f), fp(512.f)> height;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.05f)> bg_r;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.05f)> bg_g;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.1f)> bg_b;
    
        ::out<GpuTexture> texture;
    } endpoints;
    void operator()(double);
    ~RenderTargetNode();
private:
    GLuint fbo_ = 0, color_ = 0, depth_ = 0;
    int    w_ = 0, h_ = 0;
};

struct TextureViewNode {
    static consteval std::string_view name() { return "texture_view"; }
    static consteval std::string_view source_header() { return "components/render_nodes/render_nodes.hpp"; }
    struct endpoints {
        in<GpuTexture> texture;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> alpha;
        out<DrawFn> render;
    } endpoints;
    void operator()(double);
private:
    GlProgram* prog();  // lazy; render thread only
    GLuint vao_ = 0;
    GpuTexture tex_{};
    std::unique_ptr<GlProgram> prog_;
};
