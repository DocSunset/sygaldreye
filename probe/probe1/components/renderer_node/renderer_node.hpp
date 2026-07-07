// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

// Built-in graph node representing the renderer.
// Its input ports accept eye-pose overrides wired from other nodes (e.g. controllers).
// App.cpp reads inputs after tick_graph and checks graph.edges to detect whether
// any left_* or right_* port has an active edge.
struct RendererNode {
    static consteval std::string_view name()          { return "renderer"; }
    static consteval std::string_view source_header() { return "components/renderer_node/renderer_node.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/renderer_node/renderer_node.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_x;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_y;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_z;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_x;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_y;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_z;
        normalled_in<float, fp(-1.f), fp(1.f), fp(1.f)> left_rot_w;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_x;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_y;
        normalled_in<float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_z;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_x;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_y;
        normalled_in<float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_z;
        normalled_in<float, fp(-1.f), fp(1.f), fp(1.f)> right_rot_w;
    } endpoints;


    void operator()(double) {}
};
