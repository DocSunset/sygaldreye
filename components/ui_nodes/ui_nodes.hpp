// Copyright 2026 Travis West
#pragma once
#include "vr_panel.hpp"
#include "rgba_shader.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>

// UI as graph nodes: widgets take a hand ray + trigger through ordinary
// edges and emit values + draw calls. A slider is a source node you patch
// into anything; its drag state is internal, so it survives live edits.
// Panels face +Z (world-aligned) for now; orientation becomes an input
// when a use case demands it.

struct UiSliderNode {
    static consteval std::string_view name() { return "ui_slider"; }
    static consteval std::string_view source_header() { return "components/ui_nodes/ui_nodes.hpp"; }
    struct inputs {
        slider<"x",     "", float, fp(-5.f),    fp(5.f),    fp(0.f)>    x;
        slider<"y",     "", float, fp(-5.f),    fp(5.f),    fp(1.2f)>   y;
        slider<"z",     "", float, fp(-5.f),    fp(5.f),    fp(-0.8f)>  z;
        slider<"width", "", float, fp(0.05f),   fp(2.f),    fp(0.3f)>   width;
        slider<"min",   "", float, fp(-1000.f), fp(1000.f), fp(0.f)>    min;
        slider<"max",   "", float, fp(-1000.f), fp(1000.f), fp(1.f)>    max;
        slider<"init",  "", float, fp(-1000.f), fp(1000.f), fp(0.5f)>   init;
        port<"ray_pos", Eigen::Vector3f>    ray_pos;
        port<"ray_rot", Eigen::Quaternionf> ray_rot;
        slider<"trigger", "", float, fp(0.f), fp(1.f), fp(0.f)> trigger;
    } inputs;
    struct outputs {
        port<"value",  float>  value;
        port<"render", DrawFn> render;
    } outputs;
    void operator()(double);
private:
    float      value_norm_ = -1.f;  // 0..1; <0: initialize from `init`
    bool       hover_      = false;
    RgbaShader shader_;
    bool       shader_ready_ = false;
};

struct UiButtonNode {
    static consteval std::string_view name() { return "ui_button"; }
    static consteval std::string_view source_header() { return "components/ui_nodes/ui_nodes.hpp"; }
    struct inputs {
        slider<"x", "", float, fp(-5.f),  fp(5.f), fp(0.f)>   x;
        slider<"y", "", float, fp(-5.f),  fp(5.f), fp(1.f)>   y;
        slider<"z", "", float, fp(-5.f),  fp(5.f), fp(-0.8f)> z;
        slider<"width",  "", float, fp(0.02f), fp(1.f), fp(0.12f)> width;
        slider<"height", "", float, fp(0.02f), fp(1.f), fp(0.06f)> height;
        port<"ray_pos", Eigen::Vector3f>    ray_pos;
        port<"ray_rot", Eigen::Quaternionf> ray_rot;
        slider<"trigger", "", float, fp(0.f), fp(1.f), fp(0.f)> trigger;
    } inputs;
    struct outputs {
        port<"pressed", float>  pressed;
        port<"hover",   float>  hover;
        port<"render",  DrawFn> render;
    } outputs;
    void operator()(double);
private:
    bool       hover_ = false, pressed_ = false;
    RgbaShader shader_;
    bool       shader_ready_ = false;
};

struct UiPaneNode {
    static consteval std::string_view name() { return "ui_pane"; }
    static consteval std::string_view source_header() { return "components/ui_nodes/ui_nodes.hpp"; }
    struct inputs {
        slider<"x", "", float, fp(-5.f), fp(5.f), fp(0.f)>   x;
        slider<"y", "", float, fp(-5.f), fp(5.f), fp(1.2f)>  y;
        slider<"z", "", float, fp(-5.f), fp(5.f), fp(-0.81f)> z;
        slider<"width",  "", float, fp(0.05f), fp(3.f), fp(0.5f)>  width;
        slider<"height", "", float, fp(0.05f), fp(3.f), fp(0.4f)>  height;
        slider<"r", "", float, fp(0.f), fp(1.f), fp(0.07f)> r;
        slider<"g", "", float, fp(0.f), fp(1.f), fp(0.09f)> g;
        slider<"b", "", float, fp(0.f), fp(1.f), fp(0.13f)> b;
        slider<"alpha", "", float, fp(0.f), fp(1.f), fp(0.9f)> alpha;
    } inputs;
    struct outputs {
        port<"render", DrawFn> render;
    } outputs;
    void operator()(double);
private:
    RgbaShader shader_;
    bool       shader_ready_ = false;
};
