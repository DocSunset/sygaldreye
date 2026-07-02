// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <string_view>

#include "render_payloads.hpp"  // Mesh, Surface, Shader
#include "sygaldry_endpoints.hpp"

// A narrow stick attached to a controller pose. Near-field interaction:
// widgets test against tip_pos instead of a ray. Emits tip_pos plus a
// declarative Mesh+Surface (unlit, uniform-colored box) consumed by a draw
// node — GL lives only in render_region.
class PokeStickNode {
public:
    static consteval std::string_view name() { return "poke_stick"; }
    static consteval std::string_view source_header() {
        return "components/poke_stick/poke_stick.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/poke_stick/poke_stick.cpp";
    }

    struct endpoints {
        in<Eigen::Vector3f> pos;
        in<Eigen::Quaternionf> rot;
        normalled_in<float, fp(0.05f), fp(0.5f), fp(0.15f)> length;
        normalled_in<float, fp(0.002f), fp(0.02f), fp(0.005f)> radius;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> active;
        ::out<Eigen::Vector3f> tip_pos;
        ::out<Surface> surface;
        ::out<Mesh> mesh;
    } endpoints;

    void operator()(double time_s);

private:
    Shader shader_;
    std::shared_ptr<TriMeshData> data_;  // mutated in place + touch()ed
};
