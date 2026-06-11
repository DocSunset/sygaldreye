// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>

// A narrow stick attached to a controller pose. Near-field interaction:
// widgets test against tip_pos instead of a ray. First live-shipped plugin.
class PokeStickNode {
public:
    static consteval std::string_view name() { return "poke_stick"; }

    struct inputs {
        port<"pos", Eigen::Vector3f>    pos;
        port<"rot", Eigen::Quaternionf> rot;
        slider<"length", "m", float, fp(0.05f), fp(0.5f), fp(0.15f)> length;
        slider<"radius", "m", float, fp(0.002f), fp(0.02f), fp(0.005f)> radius;
        slider<"active", "", float, fp(0.f), fp(1.f), fp(0.f)> active;
    } inputs;

    struct outputs {
        port<"tip_pos", Eigen::Vector3f> tip_pos;
        port<"render",  DrawFn>          render;
    } outputs;

    void operator()(double time_s);

private:
    unsigned program_ = 0, vbo_ = 0, vao_ = 0;
    bool gl_ready_ = false;
    void ensure_gl();   // render thread only (runs inside the DrawFn)
};
