// Copyright 2026 Travis West
#pragma once
#include "gpu_texture.hpp"
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <GLES3/gl3.h>
#include <memory>
#include <string>
#include <string_view>

// A live-codable fullscreen shader pass: texture in, texture out, fragment
// code in a text param. The param supplies the body of
//     vec4 effect(vec2 uv)
// with uTex (input), uTime, and patchable uniforms uA..uD in scope.
// Recompiles whenever the code changes — shaders become graph data, ship
// in JSON, and chain: effect → effect → effect.
struct GlslEffectNode {
    static consteval std::string_view name() { return "glsl_effect"; }
    static consteval std::string_view source_header() { return "components/glsl_effect/glsl_effect.hpp"; }

    struct endpoints {
        in<GpuTexture> texture;
        in<GpuTexture> texture2;  // uTex2: mix/composite input
        normalled_in<std::string> code;  // body of effect(); empty = passthrough
        normalled_in<float, fp(-10.f), fp(10.f), fp(0.f)> a, b, c, d;
        normalled_in<float, fp(64.f), fp(2048.f), fp(512.f)> width, height;
        out<GpuTexture> texture_out;  // processor convention: in texture, out texture_out
    } endpoints;

    void operator()(double time_s);
    ~GlslEffectNode();

private:
    bool ensure_program();
    bool ensure_target(int w, int h);

    std::unique_ptr<GlProgram> prog_;
    std::string compiled_code_;
    bool        compile_failed_ = false;
    // Ping-pong: writing while sampling our own previous output (feedback
    // self-edges) is undefined GL; alternate targets each tick instead.
    GLuint fbo_[2] = {0, 0}, color_[2] = {0, 0};
    GLuint vao_ = 0;
    int    ping_ = 0, w_ = 0, h_ = 0;
};
