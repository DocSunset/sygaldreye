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

    struct endpoints {
        normalled_in<float, fp(0.f), fp(0.1f), fp(0.055f)> feed;
        normalled_in<float, fp(0.f), fp(0.1f), fp(0.062f)> kill;
        normalled_in<float, fp(1.f), fp(32.f), fp(8.f)> steps_per_frame;
    
        ::out<GpuTexture> concentration;  // GL_RG32F
    } endpoints;


    void operator()(double time_s);
    void init(int width, int height);

private:
    GLuint fbo_[2]  = {0, 0};
    GLuint tex_[2]  = {0, 0};
    GLuint prog_    = 0;
    GLuint quad_vbo_ = 0;
    GLuint vao_      = 0;
    int    ping_    = 0;
    int    width_   = 256;
    int    height_  = 256;
    bool   ready_   = false;

    void compile_shader();
    void seed_textures();
};
