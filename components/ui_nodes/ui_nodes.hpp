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
    struct endpoints {
        normalled_in<float, fp(-5.f),    fp(5.f),    fp(0.f)>    x;
        normalled_in<float, fp(-5.f),    fp(5.f),    fp(1.2f)>   y;
        normalled_in<float, fp(-5.f),    fp(5.f),    fp(-0.8f)>  z;
        normalled_in<float, fp(0.05f),   fp(2.f),    fp(0.3f)>   width;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)>    min;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(1.f)>    max;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.5f)>   init;
        ::in<Eigen::Vector3f>    ray_pos;
        ::in<Eigen::Quaternionf> ray_rot;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> trigger;
        ::out<float>  value;
        ::out<DrawFn> render;
    } endpoints;
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
    struct endpoints {
        normalled_in<float, fp(-5.f),  fp(5.f), fp(0.f)>   x;
        normalled_in<float, fp(-5.f),  fp(5.f), fp(1.f)>   y;
        normalled_in<float, fp(-5.f),  fp(5.f), fp(-0.8f)> z;
        normalled_in<float, fp(0.02f), fp(1.f), fp(0.12f)> width;
        normalled_in<float, fp(0.02f), fp(1.f), fp(0.06f)> height;
        ::in<Eigen::Vector3f>    ray_pos;
        ::in<Eigen::Quaternionf> ray_rot;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> trigger;
        ::out<float>  pressed;
        ::out<float>  hover;
        ::out<DrawFn> render;
    } endpoints;
    void operator()(double);
private:
    bool       hover_ = false, pressed_ = false;
    RgbaShader shader_;
    bool       shader_ready_ = false;
};

struct UiPaneNode {
    static consteval std::string_view name() { return "ui_pane"; }
    static consteval std::string_view source_header() { return "components/ui_nodes/ui_nodes.hpp"; }
    struct endpoints {
        normalled_in<float, fp(-5.f),  fp(5.f), fp(0.f)>    x;
        normalled_in<float, fp(-5.f),  fp(5.f), fp(1.2f)>   y;
        normalled_in<float, fp(-5.f),  fp(5.f), fp(-0.81f)> z;
        normalled_in<float, fp(0.05f), fp(3.f), fp(0.5f)>   width;
        normalled_in<float, fp(0.05f), fp(3.f), fp(0.4f)>   height;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.07f)> r;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.09f)> g;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.13f)> b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.9f)>  alpha;
        ::out<DrawFn> render;
    } endpoints;
    void operator()(double);
private:
    RgbaShader shader_;
    bool       shader_ready_ = false;
};
