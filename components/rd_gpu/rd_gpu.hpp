// Copyright 2025 Travis West
#pragma once
#include "gpu_texture.hpp"
#include "sygaldry_endpoints.hpp"
#include <GLES3/gl3.h>
#include <string_view>

// GPU Gray-Scott reaction-diffusion via ping-pong FBOs.
// Call init() once after the GL context is available.
struct RdGpu {
    static consteval std::string_view name() { return "rd_gpu"; }

    struct inputs {
        slider<"feed",   "", float, 0.f, 0.1f,  0.055f> feed;
        slider<"kill",   "", float, 0.f, 0.1f,  0.062f> kill;
        slider<"steps",  "", float, 1.f, 32.f,  8.f>    steps_per_frame;
    } inputs;

    struct outputs {
        port<"concentration", GpuTexture> concentration;  // GL_RG32F
    } outputs;

    void operator()(double time_s);
    void init(int width, int height);

private:
    GLuint fbo_[2]  = {0, 0};
    GLuint tex_[2]  = {0, 0};
    GLuint prog_    = 0;
    GLuint quad_vbo_ = 0;
    int    ping_    = 0;
    int    width_   = 256;
    int    height_  = 256;
    bool   ready_   = false;

    void compile_shader();
    void seed_textures();
};
