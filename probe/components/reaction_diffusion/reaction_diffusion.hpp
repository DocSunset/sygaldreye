// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"
#include "tri_mesh.hpp"

struct RDParams {
    int             grid_w = 256, grid_h = 256;
    float           Du = 0.16f, Dv = 0.08f, F = 0.060f, k = 0.062f;
    float           dt = 1.0f;
    int             steps_per_frame = 8;
    Eigen::Vector4f color_a = {0.05f, 0.15f, 0.35f, 1.0f};  // deep blue
    Eigen::Vector4f color_b = {0.95f, 0.90f, 0.70f, 1.0f};  // cream
};

// reaction_diffusion: a CPU Gray-Scott sim painted onto an XZ plate. The sim
// stays a node; the DRAW leaves. Positions are fixed; per-vertex colors (from
// the V field) change each frame -> dynamic vertex-colored Mesh + unlit
// Surface. GL lives in render_region.
class ReactionDiffusion {
   public:
    static consteval std::string_view name() { return "reaction_diffusion"; }
    static consteval std::string_view source_header() {
        return "components/reaction_diffusion/reaction_diffusion.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/reaction_diffusion/reaction_diffusion.cpp";
    }

    struct endpoints {
        normalled_in<float, fp(0.0f), fp(1.0f), fp(0.16f)> Du;
        normalled_in<float, fp(0.0f), fp(1.0f), fp(0.08f)> Dv;
        normalled_in<float, fp(0.0f), fp(0.1f), fp(0.06f)> F;
        normalled_in<float, fp(0.0f), fp(0.1f), fp(0.062f)> k;
        normalled_in<float, fp(0.1f), fp(2.0f), fp(1.0f)> dt;
        normalled_in<float, fp(1.0f), fp(32.0f), fp(8.0f)> steps_per_frame;
        ::out<Surface> surface;
        ::out<Mesh>    mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    void init();
    void step_sim();

    RDParams                     params_;
    std::vector<float>           u_, v_, u_next_, v_next_;
    std::shared_ptr<TriMeshData> data_;
    Shader                       shader_;
};
