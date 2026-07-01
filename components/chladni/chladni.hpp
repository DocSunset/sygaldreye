// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"
#include "tri_mesh.hpp"

// chladni: a flat plate whose per-vertex colors trace a morphing Chladni nodal
// pattern (unlit — the pattern IS the color). Positions/indices are built once;
// only colors change each frame, so render_region re-uploads vertices into a
// reused buffer. GL lives in render_region.
class Chladni {
   public:
    static consteval std::string_view name() { return "chladni"; }
    static consteval std::string_view source_header() { return "components/chladni/chladni.hpp"; }
    static consteval std::string_view source_cpp() { return "components/chladni/chladni.cpp"; }

    struct endpoints {
        normalled_in<float, fp(0.0f), fp(20.0f), fp(1.0f)> omega;
        ::out<Surface> surface;
        ::out<Mesh>    mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    void init();
    std::shared_ptr<TriMeshData> data_;
    Shader                       shader_;
    int                          n_ = 200;
};
