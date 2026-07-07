// Copyright 2025 Travis West
#pragma once
#include <array>
#include <Eigen/Core>
#include <memory>
#include <string_view>

#include "grab_target.hpp"
#include "render_payloads.hpp"  // Mesh, Surface, Shader
#include "sygaldry_endpoints.hpp"

// A composited VR widget as a graph node: a fixed anchor sphere connected by
// a rubber-band cylinder to a freely draggable control sphere. Two hand poses
// + grips arrive through edges; grab detection runs internally; the offset
// vector and a declarative Mesh+Surface (both spheres + the band, one mesh)
// flow out. GL lives only in render_region (ABI v8). The label that the old
// GL widget drew is dropped — text is a text_label node's job now.
struct RubberBandController {
    static consteval std::string_view name() { return "rubber_band"; }
    static consteval std::string_view source_header() {
        return "components/rubber_band_controller/rubber_band_controller.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/rubber_band_controller/rubber_band_controller.cpp";
    }

    struct endpoints {
        ::in<Eigen::Vector3f> left_pos;
        ::in<Eigen::Vector3f> right_pos;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> left_grip;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> right_grip;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> anchor_x;
        normalled_in<float, fp(-5.f), fp(5.f), fp(1.2f)> anchor_y;
        normalled_in<float, fp(-5.f), fp(5.f), fp(-0.6f)> anchor_z;
        ::out<Eigen::Vector3f> offset;  // anchor → control
        ::out<Surface> surface;
        ::out<Mesh> mesh;
    } endpoints;

    void operator()(double time_s);

private:
    std::array<GrabTarget, 2> targets_{};
    bool seeded_ = false;
    Shader shader_;
};
