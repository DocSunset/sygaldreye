// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "render_payloads.hpp"  // MeshPtr, Surface, Mesh, Shader
#include "sygaldry_endpoints.hpp"

// sky_dome: a shader-specific node. Emits a unit dome (Mesh) + the sky gradient
// / sun-disc Surface (drawn as background: depth off, no cull). The vertex
// shader's w=0 + xyww trick pins it to the far plane and makes radius cancel,
// so the dome follows the camera. Keeps its sun_dir/sun_color/elevation outputs
// for downstream lighting (vertex_color_mesh, water). GL left this node.
struct SkyDome {
    static consteval std::string_view name() { return "sky_dome"; }
    static consteval std::string_view source_header() {
        return "components/sky_dome/sky_dome.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/sky_dome/sky_dome.cpp";
    }

    struct endpoints {
        normalled_in<float, fp(-1.0f), fp(1.0f), fp(0.5f)> sun_elevation;
        normalled_in<float, fp(10.0f), fp(2000.0f), fp(500.0f)> radius;  // vestigial; cancels
        ::out<Surface>         surface;
        ::out<Mesh>            mesh;
        ::out<float>           sun_elevation_out;
        ::out<float>           sun_azimuth_out;
        ::out<Eigen::Vector3f> sun_dir;
        ::out<Eigen::Vector4f> sun_color;
    } endpoints;

    void operator()(double time_s);

   private:
    MeshPtr dome_;
    Shader  shader_;
};
