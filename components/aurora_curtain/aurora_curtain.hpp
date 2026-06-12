// Copyright 2026 Travis West
#pragma once
#include "gl_geometry.hpp"
#include "gl_program.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <memory>
#include <string_view>

// One aurora curtain: a rippling translucent sheet, additively blended.
// An aurora is a subgraph of these (assets/graphs/aurora.json) — color,
// placement, and motion are per-node params, all patchable.
struct AuroraCurtainNode {
    static consteval std::string_view name() { return "aurora_curtain"; }
    static consteval std::string_view source_header() { return "components/aurora_curtain/aurora_curtain.hpp"; }

    struct endpoints {
        normalled_in<float, fp(0.f),   fp(1.f),    fp(0.1f)>  r;
        normalled_in<float, fp(0.f),   fp(1.f),    fp(0.9f)>  g;
        normalled_in<float, fp(0.f),   fp(1.f),    fp(0.3f)>  b;
        normalled_in<float, fp(-500.f),fp(500.f),  fp(0.f)>   x_offset;
        normalled_in<float, fp(0.f),   fp(6.283f), fp(0.f)>   phase;
        normalled_in<float, fp(0.1f),  fp(3.f),    fp(1.f)>   freq;
        normalled_in<float, fp(0.f),   fp(3.f),    fp(0.7f)>  speed;
        normalled_in<float, fp(0.f),   fp(200.f),  fp(60.f)>  alt_base;
        normalled_in<float, fp(10.f),  fp(500.f),  fp(170.f)> alt_height;
        normalled_in<float, fp(0.f),   fp(100.f),  fp(22.f)>  ripple_amp;
        normalled_in<float, fp(50.f),  fp(1000.f), fp(400.f)> depth;
        ::out<DrawFn> render;
    } endpoints;

    void operator()(double time_s);

private:
    void init_gl();
    std::unique_ptr<GlProgram> prog_;
    GlGeometry geom_;
    GLsizei    index_count_ = 0;
    float      time_s_      = 0.f;
};
