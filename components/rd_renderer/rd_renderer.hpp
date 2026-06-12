// Copyright 2025 Travis West
#pragma once
#include "gpu_texture.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <string_view>

#include <GLES3/gl3.h>

class RDRenderer {
public:
    static consteval std::string_view name()          { return "rd_renderer"; }
    static consteval std::string_view source_header() { return "components/rd_renderer/rd_renderer.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/rd_renderer/rd_renderer.cpp"; }

    struct endpoints {
        ::in<GpuTexture> texture;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.05f)> r_a;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.15f)> g_a;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.35f)> b_a;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.95f)> r_b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.90f)> g_b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.70f)> b_b;
    
        ::out<DrawFn> render;
    } endpoints;


    static RDRenderer create();
    void operator()(double);
    // vp is ignored — this is a fullscreen quad; no world transform needed
    void draw(Eigen::Matrix4f const& vp) const;

    RDRenderer() = default;
    ~RDRenderer();
    RDRenderer(const RDRenderer&) = delete;
    RDRenderer& operator=(const RDRenderer&) = delete;
    RDRenderer(RDRenderer&&) noexcept;
    RDRenderer& operator=(RDRenderer&&) noexcept;

private:
    struct RawTag {};
    explicit RDRenderer(RawTag) {}
    GLuint prog_       = 0;
    GLuint vao_        = 0;
    GLint  tex_loc_    = -1;
    GLint  color_a_loc_= -1;
    GLint  color_b_loc_= -1;
    GpuTexture texture_{};
    Eigen::Vector3f color_a_{0.05f, 0.15f, 0.35f};
    Eigen::Vector3f color_b_{0.95f, 0.90f, 0.70f};
};
