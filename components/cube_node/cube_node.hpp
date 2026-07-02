// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "render_payloads.hpp"  // Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// cube: a single lit, uniform-colored box at pos/scale, with a wireable light.
// Emits a Mesh (box geometry, pos+scale baked) + a lit Surface — GL lives in
// render_region.
class CubeNode {
   public:
    static consteval std::string_view name() { return "cube"; }
    static consteval std::string_view source_header() {
        return "components/cube_node/cube_node.hpp";
    }
    static consteval std::string_view source_cpp() { return "components/cube_node/cube_node.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)>  pos_x;
        normalled_in<float, fp(-50.f), fp(50.f), fp(0.f)>  pos_y;
        normalled_in<float, fp(-50.f), fp(50.f), fp(-2.f)> pos_z;
        normalled_in<float, fp(0.01f), fp(20.f), fp(0.5f)> scale;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)> color_r, color_g, color_b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> roughness;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)>  metalness;
        ::in<Eigen::Vector3f> light_dir;
        ::in<Eigen::Vector3f> light_color;
        ::in<float>           light_intensity;
        ::out<Surface>        surface;
        ::out<Mesh>           mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    Shader shader_;
    MeshPtr mesh_;  // rebuilt only when pos/scale change
    Eigen::Vector3f last_pos_{0.f, 0.f, 0.f};
    float last_scale_ = 0.f;
};
