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

    struct endpoints {
        normalled_in<float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_x;
        normalled_in<float, fp(-50.f), fp(50.f),  fp(0.f)>  pos_y;
        normalled_in<float, fp(-50.f), fp(50.f),  fp(-2.f)> pos_z;
        normalled_in<float, fp(0.01f), fp(20.f),  fp(0.5f)> scale;
        normalled_in<float, fp(0.f),   fp(1.f),   fp(0.8f)> color_r, color_g, color_b;
        normalled_in<float, fp(0.f),   fp(1.f),   fp(0.5f)> roughness;
        normalled_in<float, fp(0.f),   fp(1.f),   fp(0.f)>  metalness;
        ::in<Eigen::Vector3f> light_dir;
        ::in<Eigen::Vector3f> light_color;
        ::in<float>           light_intensity;
        ::out<DrawFn> render;
    } endpoints;

    void operator()(double time_s);

private:
    CubeMesh mesh_;
    bool     initialized_ = false;
};
