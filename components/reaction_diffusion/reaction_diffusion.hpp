// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>
#include <vector>

struct RDParams {
    int   grid_w = 256, grid_h = 256;
    float Du = 0.16f, Dv = 0.08f, F = 0.060f, k = 0.062f;
    float dt = 1.0f;
    int   steps_per_frame = 8;
    Eigen::Vector4f color_a = {0.05f, 0.15f, 0.35f, 1.0f}; // deep blue
    Eigen::Vector4f color_b = {0.95f, 0.90f, 0.70f, 1.0f}; // cream
};

class ReactionDiffusion {
public:
    static consteval std::string_view name()          { return "reaction_diffusion"; }
    static consteval std::string_view source_header() { return "components/reaction_diffusion/reaction_diffusion.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/reaction_diffusion/reaction_diffusion.cpp"; }

    struct inputs {
        slider<"Du",             "", float, fp(0.0f), fp(1.0f),  fp(0.16f)> Du;
        slider<"Dv",             "", float, fp(0.0f), fp(1.0f),  fp(0.08f)> Dv;
        slider<"F",              "", float, fp(0.0f), fp(0.1f),  fp(0.06f)> F;
        slider<"k",              "", float, fp(0.0f), fp(0.1f),  fp(0.062f)> k;
        slider<"dt",             "", float, fp(0.1f), fp(2.0f),  fp(1.0f)>  dt;
        slider<"steps per frame","", float, fp(1.0f), fp(32.0f), fp(8.0f)>  steps_per_frame;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    ReactionDiffusion() { *this = create_default(); }

    static ReactionDiffusion create(RDParams const&);
    void update();
    void operator()(double time_s);
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    struct RawTag {};
    explicit ReactionDiffusion(RawTag) {}
    static ReactionDiffusion create_default();
    RDParams                   params_;
    std::vector<float>         u_, v_, u_next_, v_next_;
    TriMeshData                data_;
    TriMesh                    mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint                      mvp_loc_ = -1;
};
