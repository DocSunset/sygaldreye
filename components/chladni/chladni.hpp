// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>

struct ChladniParams {
    int   grid_n   = 256;
    int   mode_m   = 3;
    int   mode_n   = 4;
    float omega    = 1.0f;
    Eigen::Vector4f node_color  = {0.95f, 0.90f, 0.75f, 1.0f}; // sand/cream
    Eigen::Vector4f anti_color  = {0.05f, 0.08f, 0.18f, 1.0f}; // dark blue
};

class Chladni {
public:
    static consteval std::string_view name()          { return "chladni"; }
    static consteval std::string_view source_header() { return "components/chladni/chladni.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/chladni/chladni.cpp"; }

    struct inputs {
        slider<"omega", "", float, fp(0.0f), fp(20.0f), fp(1.0f)> omega;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    Chladni() { *this = create_default(); }

    static Chladni create(ChladniParams const&);
    void update(float time_s);
    void operator()(double time_s);
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    struct RawTag {};
    explicit Chladni(RawTag) {}
    static Chladni create_default();
    ChladniParams params_;
    TriMeshData   data_;
    TriMesh       mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
