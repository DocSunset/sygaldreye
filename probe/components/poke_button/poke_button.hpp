// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <memory>
#include <string_view>

#include "render_payloads.hpp"  // Mesh, Surface, Shader
#include "sygaldry_endpoints.hpp"

// Collision-based UI, fully graph-expressed: a button in space that fires
// when a poke_stick tip is inside it and the press input rises. Emits its
// interaction outputs (pressed/hover) plus a declarative Mesh+Surface (an
// unlit, uniform-color box) consumed by a draw node — GL lives in
// render_region. Wiring its bang into a spawner grows the graph itself:
//   stick.tip_pos → button.tip ; controller.trigger → button.press ;
//   button.pressed → spawner.trigger.
struct PokeButtonNode {
    static consteval std::string_view name() { return "poke_button"; }
    static consteval std::string_view source_header() {
        return "components/poke_button/poke_button.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/poke_button/poke_button.cpp";
    }

    struct endpoints {
        in<Eigen::Vector3f> tip;  // probe point (stick tip)
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> press;
        normalled_in<float, fp(-10.f), fp(10.f), fp(0.f)> x;
        normalled_in<float, fp(-10.f), fp(10.f), fp(1.2f)> y;
        normalled_in<float, fp(-10.f), fp(10.f), fp(-0.5f)> z;
        normalled_in<float, fp(0.01f), fp(0.5f), fp(0.06f)> size;
        event_out pressed;   // tip inside AND press rising
        ::out<float> hover;  // 1 while the tip is inside
        ::out<Surface> surface;
        ::out<Mesh> mesh;
    } endpoints;

    void operator()(double time_s);

private:
    Shader shader_;
    bool prev_press_ = false;
};
