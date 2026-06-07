// Copyright 2025 Travis West
#pragma once
#include "cube_mesh.hpp"
#include "light.hpp"
#include "material.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <string_view>

class CubeNode {
public:
    static consteval std::string_view name() { return "cube"; }

    struct inputs {
        slider<"pos_x",         "", float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_x;
        slider<"pos_y",         "", float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_y;
        slider<"pos_z",         "", float, fp(-50.f), fp(50.f),  fp(-2.f)> pos_z;
        slider<"scale",         "", float, fp(0.01f), fp(20.f),  fp(0.5f)> scale;
        slider<"color_r",       "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_r;
        slider<"color_g",       "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_g;
        slider<"color_b",       "", float, fp(0.f),   fp(1.f),   fp(0.8f)> color_b;
        slider<"roughness",     "", float, fp(0.f),   fp(1.f),   fp(0.5f)> roughness;
        slider<"metalness",     "", float, fp(0.f),   fp(1.f),   fp(0.f)>  metalness;
        port<"light_dir",       Eigen::Vector3f>                            light_dir;
        port<"light_color",     Eigen::Vector3f>                            light_color;
        port<"light_intensity", float>                                      light_intensity;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    void operator()(double time_s);

private:
    CubeMesh mesh_;
    bool     initialized_ = false;
};
