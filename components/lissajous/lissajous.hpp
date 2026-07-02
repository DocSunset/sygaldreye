// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// lissajous: a parametric curve drawn as an unlit, per-vertex-colored
// LINE_STRIP. The curve animates (phase + z drift) over time, so the geometry
// is dynamic (rebuilt each frame). GL lives in render_region.
class Lissajous {
   public:
    static consteval std::string_view name() { return "lissajous"; }
    static consteval std::string_view source_header() {
        return "components/lissajous/lissajous.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/lissajous/lissajous.cpp"; }

    struct endpoints {
        normalled_in<float, fp(0.5f), fp(20.0f), fp(3.0f)> freq_x;
        normalled_in<float, fp(0.5f), fp(20.0f), fp(4.0f)> freq_y;
        normalled_in<float, fp(0.0f), fp(5.0f), fp(0.5f)> freq_z;
        normalled_in<float, fp(0.0f), fp(6.283f), fp(0.0f)> phase_x;
        normalled_in<float, fp(0.1f), fp(5.0f), fp(1.0f)> amp;
        normalled_in<float, fp(100.f), fp(10000.f), fp(4000.f)> samples;
        ::out<Surface> surface;
        ::out<Mesh>    mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    Shader shader_;
    std::shared_ptr<TriMeshData> data_;  // mutated in place + touch()ed
};
