// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <string_view>

// Collision-based UI, fully graph-expressed: a button in space that fires
// when a poke_stick tip is inside it and the press input rises. First
// slice of the editor recomposition — interaction without editor C++:
//   stick.tip_pos → button.tip ; controller.trigger → button.press ;
//   button.pressed → spawner.trigger (the graph grows itself).
struct PokeButtonNode {
    static consteval std::string_view name() { return "poke_button"; }
    static consteval std::string_view source_header() { return "components/poke_button/poke_button.hpp"; }

    struct endpoints {
        in<Eigen::Vector3f> tip;              // probe point (stick tip)
        normalled_in<float, fp(0.f),   fp(1.f),  fp(0.f)>    press;
        normalled_in<float, fp(-10.f), fp(10.f), fp(0.f)>    x;
        normalled_in<float, fp(-10.f), fp(10.f), fp(1.2f)>   y;
        normalled_in<float, fp(-10.f), fp(10.f), fp(-0.5f)>  z;
        normalled_in<float, fp(0.01f), fp(0.5f), fp(0.06f)>  size;
        event_out pressed;            // tip inside AND press rising
        out<float>  hover;            // 1 while the tip is inside
        out<DrawFn> render;
    } endpoints;

    void operator()(double time_s);

private:
    unsigned program_ = 0, vbo_ = 0, vao_ = 0;
    bool gl_ready_ = false;
    bool prev_press_ = false;
    void ensure_gl();
};
